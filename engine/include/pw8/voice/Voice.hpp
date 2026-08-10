#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/algorithm/AlgorithmExecutor.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/envelope/DahdsrEnvelope.hpp"
#include "pw8/operator/OperatorNode.hpp"

// A single polyphonic voice: 8 operator nodes routed through a (shared, precompiled)
// algorithm graph, one amplitude envelope, and the per-note state needed for MPE and
// voice allocation. Per docs/ROADMAP.md Phase 5+, each voice will eventually own 8
// envelopes / 8 LFOs / a mod matrix instance; this pass wires the amplitude envelope
// only and documents the rest as PLANNED so the allocation/stealing/MPE contract is
// established now rather than retrofitted.

namespace pw8::voice
{
    /// Per-note expressive state. Populated from MIDI/MPE at note-on and updated by
    /// subsequent per-channel messages. Not yet routed through a modulation matrix
    /// (Phase 5) -- currently only pitchBendSemitones directly affects pitch.
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
            dsp::DeterministicRng rng(dsp::DeterministicRng::deriveSeed(voiceSeed, id, voiceSeed ^ noteGenerationId));
            for (auto& s : operatorStates)
                s.reset(rng.nextFloat());

            ampEnvelope.noteOn(envParams);
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
                           float& outLeft, float& outRight) noexcept
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

            const float raw = executor.processSample(compiled, operatorParams, operatorStates, wavetables, effectiveFreq);
            const float env = ampEnvelope.renderSample();
            const float amp = dsp::clamp(raw * env * velocity * outputGain, -16.0f, 16.0f);

            const float panClamped = dsp::clamp(pan, -1.0f, 1.0f);
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

    private:
        double sampleRate_ = 48000.0;
    };

} // namespace pw8::voice
