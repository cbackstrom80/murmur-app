#pragma once

#include <array>
#include <cstdint>

#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"

// LFO -- per-voice low-frequency generator (docs/MODULATION.md "LFO").
//
// Status: IMPLEMENTED for per-voice use (one instance owned by each pw8::voice::Voice,
// see voice/Voice.hpp). A shared *global* LFO mode (one instance, phase-locked across
// all voices) is PLANNED -- see docs/MODULATION.md; this class itself doesn't care
// which way it's used, but only the per-voice wiring exists today.
//
// Waveforms are intentionally naive (non-band-limited): LFO rates are sub-audio by
// convention, so PolyBLEP-style correction isn't warranted here the way it is for
// pw8::oscillator::ClassicOscillator. Using this as an audio-rate modulation source
// (TempoSync at a fast division, or a manually high rateHz) will alias -- a known,
// accepted limitation for a *control*-rate generator, not a defect.

namespace pw8::lfo
{
    enum class LfoWaveform : std::uint8_t
    {
        Sine = 0,
        Triangle,
        Saw,
        Square,
        SampleHold,
        SmoothRandom,
    };

    enum class LfoMode : std::uint8_t
    {
        Free = 0,     ///< Runs continuously from voice creation; note-on does not reset phase.
        Retrigger,    ///< Phase resets to `phaseOffset` on every note-on.
        OneShot,      ///< Like Retrigger, but stops (holds its last value) after one cycle.
        TempoSync,    ///< Rate follows `syncDivisionIndex` against host BPM instead of `rateHz`.
    };

    struct LfoParams
    {
        LfoWaveform waveform = LfoWaveform::Sine;
        LfoMode mode = LfoMode::Free;
        float rateHz = 2.0f;
        /// Index into a fixed table of musical divisions, used when mode == TempoSync.
        /// See kSyncDivisionQuarterNotes below for the mapping (default index 4 == 1/4 note).
        int syncDivisionIndex = 4;
        float phaseOffset = 0.0f; ///< 0..1
    };

    /// Cycle length in quarter notes for each `syncDivisionIndex`, ascending from
    /// slowest to fastest: 4 bars, 2 bars, 1 bar, 1/2, 1/4, 1/8, 1/16, 1/4 triplet,
    /// 1/8 triplet, dotted 1/4.
    inline constexpr std::array<float, 10> kSyncDivisionQuarterNotes = {
        16.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 1.0f / 3.0f, 1.0f / 6.0f, 1.5f,
    };

    class Lfo
    {
    public:
        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0; }

        void noteOn(const LfoParams& params, std::uint64_t seed) noexcept
        {
            if (params.mode != LfoMode::Free || !started_)
                phase_ = dsp::wrapPhase(params.phaseOffset);
            finished_ = false;
            started_ = true;
            rng_.reseed(seed);
            sampleHoldValue_ = rng_.nextRange(-1.0f, 1.0f);
            smoothRandomFrom_ = smoothRandomValue_;
            smoothRandomTo_ = rng_.nextRange(-1.0f, 1.0f);
        }

        [[nodiscard]] float renderSample(const LfoParams& params, float bpm) noexcept
        {
            if (finished_)
                return heldValue_;

            const float rateHz = (params.mode == LfoMode::TempoSync) ? syncedRateHz(params, bpm) : params.rateHz;
            const float dt = static_cast<float>(rateHz / sampleRate_);

            const float value = evaluate(params.waveform, phase_);
            smoothRandomValue_ = dsp::lerp(smoothRandomFrom_, smoothRandomTo_, phase_);

            const float output = (params.waveform == LfoWaveform::SmoothRandom) ? smoothRandomValue_
                                  : (params.waveform == LfoWaveform::SampleHold) ? sampleHoldValue_
                                                                                  : value;

            const float previousPhase = phase_;
            phase_ = dsp::wrapPhase(phase_ + dt);

            const bool wrapped = phase_ < previousPhase;
            if (wrapped)
            {
                sampleHoldValue_ = rng_.nextRange(-1.0f, 1.0f);
                smoothRandomFrom_ = smoothRandomTo_;
                smoothRandomTo_ = rng_.nextRange(-1.0f, 1.0f);

                if (params.mode == LfoMode::OneShot)
                {
                    finished_ = true;
                    heldValue_ = output;
                }
            }

            return output;
        }

    private:
        [[nodiscard]] static float evaluate(LfoWaveform waveform, float phase) noexcept
        {
            switch (waveform)
            {
                case LfoWaveform::Sine: return std::sin(dsp::kTwoPi * phase);
                case LfoWaveform::Triangle: return 4.0f * std::abs(phase - 0.5f) - 1.0f;
                case LfoWaveform::Saw: return 2.0f * phase - 1.0f;
                case LfoWaveform::Square: return phase < 0.5f ? 1.0f : -1.0f;
                case LfoWaveform::SampleHold: return 0.0f;   // handled by caller (held value).
                case LfoWaveform::SmoothRandom: return 0.0f; // handled by caller (interpolated value).
            }
            return 0.0f;
        }

        [[nodiscard]] static float syncedRateHz(const LfoParams& params, float bpm) noexcept
        {
            const auto idx = static_cast<std::size_t>(
                dsp::clamp(params.syncDivisionIndex, 0, static_cast<int>(kSyncDivisionQuarterNotes.size()) - 1));
            const float quarterNotesPerCycle = kSyncDivisionQuarterNotes[idx];
            const float quarterNotesPerSecond = dsp::clamp(bpm, 20.0f, 999.0f) / 60.0f;
            return quarterNotesPerSecond / quarterNotesPerCycle;
        }

        double sampleRate_ = 48000.0;
        float phase_ = 0.0f;
        bool started_ = false;
        bool finished_ = false;
        float heldValue_ = 0.0f;
        float sampleHoldValue_ = 0.0f;
        float smoothRandomValue_ = 0.0f;
        float smoothRandomFrom_ = 0.0f;
        float smoothRandomTo_ = 0.0f;
        dsp::DeterministicRng rng_;
    };

} // namespace pw8::lfo
