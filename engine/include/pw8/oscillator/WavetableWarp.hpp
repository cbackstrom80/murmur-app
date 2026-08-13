#pragma once

#include "pw8/dsp/Math.hpp"
#include "pw8/oscillator/PhaseWarpCommon.hpp"

// Wavetable engine pre-read phase warps — see docs/DESIGN_AND_WARPS_PLAN.md §3.
// Graph SYNC edges remain authoritative for carrier phase resets (docs/adr/wt-sync-precedence.md).

namespace pw8::oscillator
{
    struct WtWarpParams
    {
        float bend = 0.0f;      ///< -1..1 phase curvature before table read
        float asymmetry = 0.0f; ///< -1..1 half-cycle skew
        float syncRatio = 1.0f;  ///< 1..16 internal sync cycles per carrier cycle
        float syncAmount = 0.0f; ///< 0..1 soft/hard sync blend
        float formantShift = 0.0f; ///< -1..1 post-read emphasis (Week 5+)
    };

    /// Soft/hard sync on read phase — at amount=0 identical to input; at 1, frac(t * ratio).
    [[nodiscard]] inline float applyPhaseSync(float t, float syncRatio, float syncAmount) noexcept
    {
        const float amount = dsp::clamp(syncAmount, 0.0f, 1.0f);
        if (amount <= 0.0f)
            return t;
        const float ratio = dsp::clamp(syncRatio, 1.0f, 16.0f);
        const float synced = t * ratio - std::floor(t * ratio);
        return dsp::lerp(t, synced, amount);
    }

    /// Warp the read phase before mip/table lookup.
    [[nodiscard]] inline float warpReadPhase(float phase, const WtWarpParams& params) noexcept
    {
        const float t = dsp::wrapPhase(phase);
        const float bent = warpPhaseBendAsym(t, params.bend, params.asymmetry);
        return applyPhaseSync(bent, params.syncRatio, params.syncAmount);
    }

} // namespace pw8::oscillator
