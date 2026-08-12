#pragma once

#include <cmath>

#include "pw8/lfo/Lfo.hpp"

namespace pw8::plugin::ui::wireframe
{
    /// Static LFO waveform sampling for UI wireframes — mirrors lfo::Lfo::evaluate.
    inline float sampleLfoWaveform(lfo::LfoWaveform waveform, float phase) noexcept
    {
        switch (waveform)
        {
            case lfo::LfoWaveform::Sine:
                return std::sin(phase * 6.28318530718f);
            case lfo::LfoWaveform::Triangle:
                return 4.0f * std::abs(phase - 0.5f) - 1.0f;
            case lfo::LfoWaveform::Saw:
                return 2.0f * phase - 1.0f;
            case lfo::LfoWaveform::Square:
                return phase < 0.5f ? 1.0f : -1.0f;
            default:
                return 0.0f;
        }
    }

    /// Stepped S&H preview — deterministic from phase segment index.
    inline float sampleLfoSampleHold(float phase, int segments = 8) noexcept
    {
        const int idx = static_cast<int>(phase * static_cast<float>(segments)) % segments;
        const float t = static_cast<float>(idx) * 0.137f;
        return std::sin(t * 12.9898f) * 0.85f;
    }

    /// Smooth-random preview — sinusoidal blend between pseudo-random targets.
    inline float sampleLfoSmoothRandom(float phase, int segment = 0) noexcept
    {
        const float from = std::sin(static_cast<float>(segment) * 2.41f) * 0.7f;
        const float to = std::sin(static_cast<float>(segment + 3) * 1.73f) * 0.7f;
        const float localT = phase - std::floor(phase);
        const float smooth = localT * localT * (3.0f - 2.0f * localT);
        return from + (to - from) * smooth;
    }

} // namespace pw8::plugin::ui::wireframe
