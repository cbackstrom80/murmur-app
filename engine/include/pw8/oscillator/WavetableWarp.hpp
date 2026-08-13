#pragma once

#include "pw8/dsp/Math.hpp"
#include "pw8/oscillator/PhaseWarpCommon.hpp"

// Wavetable engine pre-read phase warps (bend + asymmetry in Week 1).
// Sync and formant warps land in later weeks — see docs/DESIGN_AND_WARPS_PLAN.md §3.

namespace pw8::oscillator
{
    struct WtWarpParams
    {
        float bend = 0.0f;      ///< -1..1 phase curvature before table read
        float asymmetry = 0.0f; ///< -1..1 half-cycle skew
        float syncRatio = 1.0f; ///< 1..16 (Week 5 integration)
        float syncAmount = 0.0f; ///< 0..1 soft/hard sync blend (Week 5)
        float formantShift = 0.0f; ///< -1..1 post-read emphasis (Week 6)
    };

    /// Warp the read phase before mip/table lookup. Identity when bend and asymmetry are zero.
    [[nodiscard]] inline float warpReadPhase(float phase, const WtWarpParams& params) noexcept
    {
        const float t = dsp::wrapPhase(phase);
        return warpPhaseBendAsym(t, params.bend, params.asymmetry);
    }

} // namespace pw8::oscillator
