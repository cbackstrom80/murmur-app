#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace pw8::dsp
{
    inline constexpr float kPi = std::numbers::pi_v<float>;
    inline constexpr float kTwoPi = 2.0f * kPi;

    template <typename T>
    [[nodiscard]] constexpr T clamp(T v, T lo, T hi) noexcept
    {
        return std::clamp(v, lo, hi);
    }

    template <typename T>
    [[nodiscard]] constexpr T lerp(T a, T b, T t) noexcept
    {
        return a + (b - a) * t;
    }

    /// Wrap a phase value into [0, 1).
    [[nodiscard]] inline float wrapPhase(float phase) noexcept
    {
        phase -= std::floor(phase);
        return phase;
    }

    [[nodiscard]] inline float dbToGain(float db) noexcept
    {
        return std::pow(10.0f, db * 0.05f);
    }

    [[nodiscard]] inline float gainToDb(float gain) noexcept
    {
        return 20.0f * std::log10(std::max(gain, 1.0e-9f));
    }

    /// MIDI note number (69 = A4) to frequency in Hz, standard 12-TET, A4 = 440 Hz.
    /// This is a fallback used before a TuningService is wired in.
    [[nodiscard]] inline float noteToFrequency12Tet(float note, float a4Hz = 440.0f) noexcept
    {
        return a4Hz * std::pow(2.0f, (note - 69.0f) / 12.0f);
    }

    /// Returns true if the value is finite and within a generous sanity bound.
    /// Used at trust boundaries (patch load, graph compile) -- NOT intended for
    /// per-sample use in the hot loop.
    [[nodiscard]] inline bool isSaneFloat(float v, float bound = 1.0e6f) noexcept
    {
        return std::isfinite(v) && std::abs(v) <= bound;
    }

    /// Flush a value to zero if it is a subnormal/NaN/Inf. Cheap enough for
    /// occasional use in feedback paths where FTZ/DAZ CPU flags alone aren't trusted
    /// (e.g. right after a division).
    [[nodiscard]] inline float flushIfNotFinite(float v) noexcept
    {
        return std::isfinite(v) ? v : 0.0f;
    }

    /// Drive-normalized tanh soft saturation: `driveLinear == 1` is unity gain at
    /// small signals, larger drive compresses harder without the output level
    /// climbing alongside it. Shared by every effect in pw8/effects/ that needs a
    /// cheap, stable nonlinearity (Saturation, TapeDelay's feedback drive,
    /// NodeDelay's per-node distortion).
    [[nodiscard]] inline float softSaturate(float x, float driveLinear) noexcept
    {
        const float drive = std::max(driveLinear, 1.0e-6f);
        const float norm = std::tanh(drive);
        if (norm <= 1.0e-6f)
            return x;
        return flushIfNotFinite(std::tanh(x * drive) / norm);
    }

} // namespace pw8::dsp
