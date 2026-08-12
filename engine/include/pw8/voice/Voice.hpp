#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/algorithm/AlgorithmExecutor.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/envelope/DahdsrEnvelope.hpp"
#include "pw8/filter/StateVariableFilter.hpp"
#include "pw8/lfo/Lfo.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"
#include "pw8/operator/OperatorNode.hpp"

// A single polyphonic voice: 8 operator nodes routed through a (shared, precompiled)
// algorithm graph, 8 envelopes, 8 LFOs, a mod matrix, Filter 1, and the per-note
// state needed for MPE and voice allocation.
//
// Signal chain per sample: 8 LFOs + 8 envelopes render (mod sources) -> mod matrix
// -> (operator-level-modulated) algorithm graph (each engine optionally filtered)
// -> global filter (optional) -> amplitude envelope (envelopes[0]) VCA -> pan -> output.
//
// envelopes[0]/lfos[0] are conventionally "the" amp envelope / LFO1 (envelopes[0]
// is the only one wired to the VCA and to voice lifetime -- see isFree()); all 8 of
// each are otherwise fully general-purpose mod matrix sources (docs/MODULATION.md
// "8 envelopes / 8 LFOs", reached in the GATE 5 pass, docs/ROADMAP.md). LAYER/GLOBAL
// mod scope is implemented for LFO sources only (see ModScope's doc comment in
// ModMatrixTypes.hpp) -- render::Engine ticks a separate, shared bank of 8 LFOs
// once per sample and passes the result into every voice's renderSample() call, so
// LAYER-scoped LFO routes read the same value on every voice this sample rather
// than each voice's own independent phase.

namespace pw8::voice
{
    /// Per-note expressive state. Populated from MIDI/MPE at note-on and updated by
    /// subsequent per-channel messages. pitchBendSemitones/mpePitch directly affect
    /// pitch; channelPressure/polyAftertouch/mpeSlide are mod matrix sources
    /// (ModSource::ChannelPressure/PolyAftertouch/MpeSlide).
    struct NoteExpression
    {
        float pitchBendSemitones = 0.0f;
        float channelPressure = 0.0f; ///< 0..1
        float polyAftertouch = 0.0f;  ///< 0..1
        float mpeSlide = 0.0f;        ///< 0..1, MPE "timbre"/slide (CC74) when in MPE mode
        float mpePitch = 0.0f;        ///< semitones, MPE per-note pitch bend
    };

    class Voice
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            for (auto& s : operatorStates)
                s.prepare(sampleRate);
            for (auto& e : envelopes)
                e.prepare(sampleRate);
            for (auto& l : lfos)
                l.prepare(sampleRate);
            filter1.prepare(sampleRate);
            for (auto& f : operatorFilters_)
                f.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void noteOn(int note, int channel, float velocityUnit, float baseFreqHz,
                    const std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer>& envParams,
                    std::uint64_t ageCounter, std::uint64_t noteGenerationId, std::uint64_t voiceSeed) noexcept
        {
            noteNumber = note;
            midiChannel = channel;
            velocity = dsp::clamp(velocityUnit, 0.0f, 1.0f);
            gateOn = true;
            age = ageCounter;
            noteGenId = noteGenerationId;
            baseFrequencyHz = baseFreqHz;
            expression = NoteExpression{};

            // Deterministic, seeded per-voice phase variation (see docs/DSP_ENGINE.md
            // "Voice Variation"): small, controlled, reproducible -- not free-running RNG.
            // The same seeded stream that randomizes operator phase also seeds each of
            // the 8 LFOs (consumed in order, one nextU64() per LFO) so every LFO gets a
            // decorrelated-but-reproducible seed rather than 8 copies of the same one.
            const auto seed = dsp::DeterministicRng::deriveSeed(voiceSeed, id, voiceSeed ^ noteGenerationId);
            dsp::DeterministicRng rng(seed);
            for (auto& s : operatorStates)
            {
                s.reset(rng.nextFloat());
                s.seedNoise(rng.nextU64());
                s.seedResonator(rng.nextU64());
                s.seedGranular(rng.nextU64());
            }

            for (std::size_t i = 0; i < core::kNumEnvelopesPerLayer; ++i)
                envelopes[i].noteOn(envParams[i]);
            filter1.reset();
            for (auto& f : operatorFilters_)
                f.reset();
            for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                lfos[i].noteOn(lfoParams[i], rng.nextU64());
        }

        void noteOff(float releaseVelocityUnit) noexcept
        {
            releaseVelocity = dsp::clamp(releaseVelocityUnit, 0.0f, 1.0f);
            gateOn = false;
            for (auto& e : envelopes)
                e.noteOff();
        }

        /// Immediately silences the voice with no release tail -- used only when a
        /// crossfaded steal isn't available and a hard reset is unavoidable.
        void hardKill() noexcept
        {
            for (auto& e : envelopes)
                e.reset();
            noteNumber = -1;
            gateOn = false;
        }

        [[nodiscard]] bool isFree() const noexcept { return noteNumber < 0 && !envelopes[0].isActive(); }
        [[nodiscard]] bool isReleased() const noexcept { return !gateOn; }
        [[nodiscard]] bool isSounding() const noexcept { return envelopes[0].isActive(); }
        [[nodiscard]] float amplitudeEstimate() const noexcept { return envelopes[0].getCurrentLevel() * velocity; }

        /// Renders one stereo sample. Returns silence and frees the voice once its
        /// amp envelope (envelopes[0]) has fully finished its release. `layerLfoValues`
        /// is this sample's shared, layer-wide LFO tick (see class doc comment) --
        /// computed once per sample by the caller (render::Engine), not per-voice.
        /// `liveModRoutes` is read fresh every sample from the caller (render::Engine's
        /// patch_.layerA.modRoutes), the same "no per-voice frozen copy" pattern
        /// `layerLfoValues` already uses -- unlike the envelope live-update exception
        /// documented on Engine::setEnvelopeLive(), a mod-route change (add/remove/
        /// retarget/reamount) takes effect on an already-sustaining voice the very
        /// next sample, not just the next note-on. This is what makes drag-to-modulate
        /// (docs/UI.md) feel immediate rather than needing a fresh keypress.
        void renderSample(const algorithm::CompiledAlgorithm& compiled,
                           const std::array<const oscillator::WavetableTable*, core::kNodesPerLayer>& wavetableTables,
                           float bpm, const std::array<float, core::kNumLfosPerLayer>& layerLfoValues,
                           const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& liveModRoutes,
                           float& outLeft, float& outRight) noexcept
        {
            if (noteNumber < 0 && !envelopes[0].isActive())
            {
                outLeft = 0.0f;
                outRight = 0.0f;
                return;
            }

            const float pitchBendHz = baseFrequencyHz *
                                       (std::pow(2.0f, (expression.pitchBendSemitones + expression.mpePitch) / 12.0f) - 1.0f);
            const float effectiveFreq = baseFrequencyHz + pitchBendHz;

            // Mod sources that don't depend on this sample's algorithm-graph output are
            // rendered first, so their values can modulate the operators that ARE about
            // to run this sample (e.g. LFO -> operator level tremolo).
            modulation::ModSourceValues sourceValues;
            for (std::size_t i = 0; i < core::kNumLfosPerLayer; ++i)
                sourceValues.voiceLfos[i] = lfos[i].renderSample(lfoParams[i], bpm);
            sourceValues.layerLfos = layerLfoValues;
            for (std::size_t i = 0; i < core::kNumEnvelopesPerLayer; ++i)
                sourceValues.envelopes[i] = envelopes[i].renderSample();
            const float env = sourceValues.envelopes[0]; // envelopes[0] is the amp envelope -- drives the VCA below.
            sourceValues.velocity = velocity;
            sourceValues.channelPressure = expression.channelPressure;
            sourceValues.polyAftertouch = expression.polyAftertouch;
            sourceValues.mpeSlide = expression.mpeSlide;
            sourceValues.macros = macroValues;

            const auto modOut = modulation::ModMatrixExecutor::apply(liveModRoutes, sourceValues);

            // Operator-level modulation needs a per-sample-modified params copy; the
            // multiplier defaults to 1.0 per operator when no route targets it, so this
            // is a correctness-preserving no-op for patches with no such routes (fixed
            // 8-struct stack copy, no allocation).
            std::array<op::OperatorParams, core::kNodesPerLayer> modulatedParams = operatorParams;
            for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
            {
                modulatedParams[i].level *= modOut.operatorLevelMultiplier[i];
                modulatedParams[i].wavetableFramePosition = dsp::clamp(
                    modulatedParams[i].wavetableFramePosition + modOut.operatorWavetablePositionOffset[i], 0.0f, 1.0f);
            }

            const float raw = executor.processSample(compiled, modulatedParams, operatorStates, wavetableTables,
                                                      effectiveFreq, operatorFilterParams_, operatorFilters_,
                                                      modOut.operatorFilterCutoffSemitones,
                                                      modOut.operatorFilterResonanceOffset);

            float filtered = raw;
            if (filterParams.enabled)
            {
                const float keyTrackFactor = std::pow(effectiveFreq / 261.6256f, filterParams.keyTrack);
                const float cutoffHz = filterParams.cutoffHz * keyTrackFactor *
                                        std::pow(2.0f, modOut.filterCutoffSemitones / 12.0f);
                const float resonance = dsp::clamp(filterParams.resonance + modOut.filterResonanceOffset, 0.0f, 1.0f);
                filtered = filter1.renderSample(raw, filterParams.mode, cutoffHz, resonance);
            }

            const float amp = dsp::clamp(filtered * env * velocity * outputGain, -16.0f, 16.0f);

            const float panClamped = dsp::clamp(pan + modOut.panOffset, -1.0f, 1.0f);
            const float panRad = (panClamped * 0.5f + 0.5f) * (dsp::kPi * 0.5f);
            outLeft = amp * std::cos(panRad);
            outRight = amp * std::sin(panRad);

            if (!envelopes[0].isActive())
                noteNumber = -1; // fully released -- free for reuse next allocation pass.
        }

        std::uint32_t id = 0;
        int noteNumber = -1; ///< -1 == free.
        int midiChannel = 0;
        float velocity = 0.0f;
        float releaseVelocity = 0.0f;
        bool gateOn = false;
        std::uint64_t age = 0;
        std::uint64_t noteGenId = 0;

        float baseFrequencyHz = 440.0f;
        float pan = 0.0f;
        float outputGain = 1.0f;

        NoteExpression expression{};

        std::array<op::OperatorParams, core::kNodesPerLayer> operatorParams{};
        std::array<op::OperatorState, core::kNodesPerLayer> operatorStates{};
        algorithm::AlgorithmExecutor executor{};
        std::array<envelope::DahdsrEnvelope, core::kNumEnvelopesPerLayer> envelopes{};

        filter::FilterParams filterParams{};
        filter::StateVariableFilter filter1{};

        std::array<filter::FilterParams, core::kNodesPerLayer> operatorFilterParams_{};
        std::array<filter::StateVariableFilter, core::kNodesPerLayer> operatorFilters_{};

        std::array<lfo::LfoParams, core::kNumLfosPerLayer> lfoParams{};
        std::array<lfo::Lfo, core::kNumLfosPerLayer> lfos{};

        std::array<float, 8> macroValues{};

    private:
        double sampleRate_ = 48000.0;
    };

} // namespace pw8::voice
