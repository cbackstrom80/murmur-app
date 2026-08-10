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
// algorithm graph, one amplitude envelope, one LFO, a mod matrix, Filter 1, and the
// per-note state needed for MPE and voice allocation.
//
// Signal chain per sample: LFO + envelope render (mod sources) -> mod matrix ->
// (operator-level-modulated) algorithm graph -> Filter 1 (cutoff/resonance
// modulated) -> amplitude envelope VCA -> pan (modulated) -> stereo output.
//
// Per docs/ROADMAP.md Phase 5+, each voice will eventually own 8 envelopes / 8 LFOs
// / LAYER+GLOBAL mod scope; this pass wires one amplitude envelope and one LFO
// (VOICE-scoped mod matrix) and documents the rest as PLANNED so the allocation/
// stealing/MPE contract is established now rather than retrofitted.

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
            ampEnvelope.prepare(sampleRate);
            filter1.prepare(sampleRate);
            lfo1.prepare(sampleRate);
            sampleRate_ = sampleRate;
        }

        void noteOn(int note, int channel, float velocityUnit, float baseFreqHz,
                    const envelope::DahdsrParams& envParams, std::uint64_t ageCounter,
                    std::uint64_t noteGenerationId, std::uint64_t voiceSeed) noexcept
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
            const auto seed = dsp::DeterministicRng::deriveSeed(voiceSeed, id, voiceSeed ^ noteGenerationId);
            dsp::DeterministicRng rng(seed);
            for (auto& s : operatorStates)
                s.reset(rng.nextFloat());

            ampEnvelope.noteOn(envParams);
            filter1.reset();
            lfo1.noteOn(lfoParams, seed);
        }

        void noteOff(float releaseVelocityUnit) noexcept
        {
            releaseVelocity = dsp::clamp(releaseVelocityUnit, 0.0f, 1.0f);
            gateOn = false;
            ampEnvelope.noteOff();
        }

        /// Immediately silences the voice with no release tail -- used only when a
        /// crossfaded steal isn't available and a hard reset is unavoidable.
        void hardKill() noexcept
        {
            ampEnvelope.reset();
            noteNumber = -1;
            gateOn = false;
        }

        [[nodiscard]] bool isFree() const noexcept { return noteNumber < 0 && !ampEnvelope.isActive(); }
        [[nodiscard]] bool isReleased() const noexcept { return !gateOn; }
        [[nodiscard]] bool isSounding() const noexcept { return ampEnvelope.isActive(); }
        [[nodiscard]] float amplitudeEstimate() const noexcept { return ampEnvelope.getCurrentLevel() * velocity; }

        /// Renders one stereo sample. Returns silence and frees the voice once its
        /// envelope has fully finished its release.
        void renderSample(const algorithm::CompiledAlgorithm& compiled,
                           const std::array<oscillator::WavetableView, core::kNodesPerLayer>& wavetables,
                           float bpm, float& outLeft, float& outRight) noexcept
        {
            if (noteNumber < 0 && !ampEnvelope.isActive())
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
            const float lfoValue = lfo1.renderSample(lfoParams, bpm);
            const float env = ampEnvelope.renderSample();

            modulation::ModSourceValues sourceValues;
            sourceValues.lfo1 = lfoValue;
            sourceValues.ampEnvelope = env;
            sourceValues.velocity = velocity;
            sourceValues.channelPressure = expression.channelPressure;
            sourceValues.polyAftertouch = expression.polyAftertouch;
            sourceValues.mpeSlide = expression.mpeSlide;
            sourceValues.macros = macroValues;

            const auto modOut = modulation::ModMatrixExecutor::apply(modRoutes, sourceValues);

            // Operator-level modulation needs a per-sample-modified params copy; the
            // multiplier defaults to 1.0 per operator when no route targets it, so this
            // is a correctness-preserving no-op for patches with no such routes (fixed
            // 8-struct stack copy, no allocation).
            std::array<op::OperatorParams, core::kNodesPerLayer> modulatedParams = operatorParams;
            for (std::size_t i = 0; i < core::kNodesPerLayer; ++i)
                modulatedParams[i].level *= modOut.operatorLevelMultiplier[i];

            const float raw = executor.processSample(compiled, modulatedParams, operatorStates, wavetables, effectiveFreq);

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

            if (!ampEnvelope.isActive())
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
        envelope::DahdsrEnvelope ampEnvelope{};

        filter::FilterParams filterParams{};
        filter::StateVariableFilter filter1{};

        lfo::LfoParams lfoParams{};
        lfo::Lfo lfo1{};

        core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> modRoutes{};
        std::array<float, 8> macroValues{};

    private:
        double sampleRate_ = 48000.0;
    };

} // namespace pw8::voice
