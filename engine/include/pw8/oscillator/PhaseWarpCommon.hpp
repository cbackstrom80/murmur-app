#pragma once

#include <cmath>

#include "pw8/dsp/Math.hpp"

// Shared phase-domain warp helpers used by PhaseShapeOscillator and WavetableWarp.
// Extracted so bend/asymmetry math stays in one place (docs/DESIGN_AND_WARPS_PLAN.md W1).

namespace pw8::oscillator
{
    /// Single-cycle sinusoidal warp curve — the smooth analogue of CZ "saw" DCW family.
    [[nodiscard]] inline float singleCycleWarpCurve(float t) noexcept
    {
        return std::sin(dsp::kTwoPi * t);
    }

    /// Double-cycle sinusoidal warp curve — formant-like CZ "resonant" family.
    [[nodiscard]] inline float doubleCycleWarpCurve(float t) noexcept
    {
        return std::sin(2.0f * dsp::kTwoPi * t);
    }

    /// Asymmetry biases warp magnitude toward one half of the cycle (skew, not uniform shift).
    [[nodiscard]] inline float applyAsymmetrySkew(float base, float t, float asymmetry) noexcept
    {
        const float a = dsp::clamp(asymmetry, -1.0f, 1.0f);
        return base * (1.0f + a * std::cos(dsp::kTwoPi * t));
    }

    /// Apply bend displacement to phase. `warpDepth` keeps max displacement under one cycle.
    [[nodiscard]] inline float applyPhaseBend(float t, float skewedBase, float bend,
                                               float warpDepth = 0.2f) noexcept
    {
        const float b = dsp::clamp(bend, -1.0f, 1.0f);
        return dsp::wrapPhase(t + b * skewedBase * warpDepth);
    }

    /// Bend + asymmetry on a single-cycle curve (wavetable read-phase path).
    [[nodiscard]] inline float warpPhaseBendAsym(float t, float bend, float asymmetry) noexcept
    {
        const float base = singleCycleWarpCurve(t);
        const float skewed = applyAsymmetrySkew(base, t, asymmetry);
        return applyPhaseBend(t, skewed, bend);
    }

} // namespace pw8::oscillator
