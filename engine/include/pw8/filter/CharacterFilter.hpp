#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Math.hpp"

// Filter 2 — nonlinear character filter (docs/DSP_ENGINE.md "Filter System").
// Original soft 4-pole ladder with tanh input drive; not a circuit clone.
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

        [[nodiscard]] float renderSample(float input, float cutoffHz, float resonance01, float drive01) noexcept
        {
            const float driveGain = 1.0f + dsp::clamp(drive01, 0.0f, 1.0f) * 4.0f;
            const float driven = std::tanh(input * driveGain);

            const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
            const float fc = dsp::clamp(cutoffHz, 20.0f, nyquist * 0.98f);
            const float g = std::tan(dsp::kPi * fc / static_cast<float>(sampleRate_));
            const float G = g / (1.0f + g);
            const float k = dsp::clamp(resonance01, 0.0f, 1.0f) * 4.0f;

            const float feedback = k * stage_[3];
            float x = driven - feedback;

            for (auto& s : stage_)
            {
                s += G * (x - s);
                x = s;
            }

            return dsp::flushIfNotFinite(stage_[3]);
        }

    private:
        double sampleRate_ = 48000.0;
        std::array<float, 4> stage_{};
    };

} // namespace pw8::filter
