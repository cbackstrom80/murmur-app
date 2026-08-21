#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Math.hpp"
#include "pw8/filter/StateVariableFilter.hpp" // reuses blendLpBpHp -- same morph math as Filter 1, no new UI/DSP paradigm.

// Filter 2 — nonlinear character filter (docs/DSP_ENGINE.md "Filter System").
// Original soft 4-pole ladder with tanh input drive; not a circuit clone.
//
// modeMorph (added after the initial LP-only MVP shipped) derives HP and BP taps
// from the SAME 4-stage cascade rather than adding new circuit topologies -- real,
// standard ladder-filter technique (complementary highpass = pre-cascade signal
// minus the 4-pole lowpass output; bandpass = difference between two different-
// order lowpass taps), not new DSP research. Blended via the identical
// `blendLpBpHp` LP->BP->HP curve Filter 1's morph already uses, so the two
// filters' morph knobs behave consistently to the ear.
namespace pw8::filter
{
    struct CharacterFilterParams
    {
        bool enabled = false;
        float cutoffHz = 4000.0f;
        /// 0 (mild) .. 1 (heavy self-oscillation tendency).
        float resonance = 0.3f;
        /// 0 (clean) .. 1 (tanh saturation on input).
        float drive = 0.0f;
        /// Semitone offset from modulated Filter 1 cutoff when F1 is enabled (Blades F2 tracking).
        float cutoffOffsetSemitones = 0.0f;
        /// Same semantics as Filter 1 keyTrack.
        float keyTrack = 0.0f;
        /// Same LP->BP->HP morph convention as Filter 1's modeMorph (0 = LP4, 0.5 = BP, 1 = HP).
        /// Defaults to 0.0 -- pure LP4, byte-identical to pre-morph output.
        float modeMorph = 0.0f;
    };

    class CharacterFilter
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        void reset() noexcept
        {
            stage_.fill(0.0f);
        }

        [[nodiscard]] float renderSample(float input, float cutoffHz, float resonance01, float drive01,
                                          float modeMorph = 0.0f) noexcept
        {
            const float driveGain = 1.0f + dsp::clamp(drive01, 0.0f, 1.0f) * 4.0f;
            const float driven = std::tanh(input * driveGain);

            const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
            const float fc = dsp::clamp(cutoffHz, 20.0f, nyquist * 0.98f);
            const float g = std::tan(dsp::kPi * fc / static_cast<float>(sampleRate_));
            const float G = g / (1.0f + g);
            const float k = dsp::clamp(resonance01, 0.0f, 1.0f) * 4.0f;

            const float feedback = k * stage_[3];
            const float x = driven - feedback;

            float prev = x;
            for (auto& s : stage_)
            {
                s += G * (prev - s);
                prev = s;
            }

            const float lowpass = stage_[3];
            if (modeMorph <= 0.0f)
                return dsp::flushIfNotFinite(lowpass); // fast path, matches pre-morph behavior exactly.

            // Complementary highpass at the same (4th) order: what the pre-cascade
            // signal had that the lowpass didn't keep.
            const float highpass = x - lowpass;
            // Band emphasis: difference between the 2-pole and 4-pole taps, the same
            // "difference of two lowpasses" technique the SVF's own Peak mode uses
            // (`lowpass - highpass` there; here, two different-order LP outputs).
            const float bandpass = stage_[1] - stage_[3];

            return dsp::flushIfNotFinite(blendLpBpHp(lowpass, bandpass, highpass, modeMorph));
        }

    private:
        double sampleRate_ = 48000.0;
        std::array<float, 4> stage_{};
    };

} // namespace pw8::filter
