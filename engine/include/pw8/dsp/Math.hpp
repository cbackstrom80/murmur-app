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

    /// 3-tap half-band kernel for one 2× decimation stage (symmetric, sum = 1).
    [[nodiscard]] inline float halfBandDecimate3Tap(float x0, float x1, float x2) noexcept
    {
        return 0.25f * x0 + 0.5f * x1 + 0.25f * x2;
    }

    /// One-sample-delayed tanh feedback with optional oversampling (evenly spaced
    /// sub-positions between previous and current input, half-band downsample). Used by
    /// FM self-feedback and algorithm-graph FEEDBACK edges when quality >= High.
    [[nodiscard]] inline float tanhFeedbackOs(float current, float previous, float amount,
                                              int osFactor) noexcept
    {
        if (osFactor <= 1)
            return flushIfNotFinite(std::tanh(current * amount));

        if (osFactor == 2)
        {
            const float y0 = std::tanh(previous * amount);
            const float y1 = std::tanh(dsp::lerp(previous, current, 0.5f) * amount);
            const float y2 = std::tanh(current * amount);
            return flushIfNotFinite(halfBandDecimate3Tap(y0, y1, y2));
        }

        // 4×: evaluate at boundaries + three interior sub-positions, two half-band stages.
        const float y0 = std::tanh(previous * amount);
        const float y1 = std::tanh(dsp::lerp(previous, current, 0.25f) * amount);
        const float y2 = std::tanh(dsp::lerp(previous, current, 0.5f) * amount);
        const float y3 = std::tanh(dsp::lerp(previous, current, 0.75f) * amount);
        const float y4 = std::tanh(current * amount);
        const float z0 = halfBandDecimate3Tap(y0, y1, y2);
        const float z1 = halfBandDecimate3Tap(y1, y2, y3);
        const float z2 = halfBandDecimate3Tap(y2, y3, y4);
        return flushIfNotFinite(halfBandDecimate3Tap(z0, z1, z2));
    }

    /// Back-compat wrapper: bool true == 2× oversampling.
    [[nodiscard]] inline float tanhFeedback2x(float current, float previous, float amount,
                                             bool oversample) noexcept
    {
        return tanhFeedbackOs(current, previous, amount, oversample ? 2 : 1);
    }

    /// Decimate folded nonlinear sub-samples produced at osFactor interior positions.
    [[nodiscard]] inline float decimateHalfBandFold(float previousFolded, float currentFolded, int osFactor,
                                                   const float* interiorFolded) noexcept
    {
        if (osFactor <= 1)
            return currentFolded;

        if (osFactor == 2)
        {
            const float mid = interiorFolded != nullptr ? interiorFolded[0] : currentFolded;
            return halfBandDecimate3Tap(previousFolded, mid, currentFolded);
        }

        const float s1 = interiorFolded[0];
        const float s2 = interiorFolded[1];
        const float s3 = interiorFolded[2];
        const float y0 = halfBandDecimate3Tap(previousFolded, s1, s2);
        const float y1 = halfBandDecimate3Tap(s1, s2, s3);
        const float y2 = halfBandDecimate3Tap(s2, s3, currentFolded);
        return halfBandDecimate3Tap(y0, y1, y2);
    }

} // namespace pw8::dsp
