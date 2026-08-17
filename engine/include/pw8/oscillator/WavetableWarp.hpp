#pragma once

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/oscillator/PhaseWarpCommon.hpp"

// Wavetable engine pre-read phase warps — see docs/DESIGN_AND_WARPS_PLAN.md §3.
// Graph SYNC edges remain authoritative for carrier phase resets (docs/adr/wt-sync-precedence.md).

namespace pw8::oscillator
{
    /// Frame-to-frame interpolation style when `framePosition01` sits between table frames.
    enum class WtMorphMode : int
    {
        Spectral = 0,  ///< Nearest-frame (no crossfade) — preserves per-frame spectral identity
        Formant = 1,   ///< Equal-power crossfade — smoother envelope than linear blend
        Crossfade = 2, ///< Linear frame interpolation (legacy default)
    };

    struct WtWarpParams
    {
        float bend = 0.0f;      ///< -1..1 phase curvature before table read
        float asymmetry = 0.0f; ///< -1..1 half-cycle skew
        float syncRatio = 1.0f;  ///< 1..16 internal sync cycles per carrier cycle
        float syncAmount = 0.0f; ///< 0..1 soft/hard sync blend
        float formantShift = 0.0f; ///< -1..1 post-read formant emphasis
        WtMorphMode morphMode = WtMorphMode::Crossfade;
    };

    /// Post-read two-peaking emphasis bank — shifts spectral envelope without rebaking mips.
    class FormantEmphasisBank
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            reset();
        }

        void reset() noexcept
        {
            lowShelf_.reset();
            highShelf_.reset();
            cachedShift_ = 999.0f;
        }

        [[nodiscard]] float process(float sample, float formantShift) noexcept
        {
            if (std::abs(formantShift) < 1.0e-5f)
                return sample;

            updateIfNeeded(formantShift);

            const float filtered = highShelf_.renderSample(lowShelf_.renderSample(sample));
            const float blend = std::min(std::abs(formantShift) * 0.85f, 1.0f);
            return dsp::flushIfNotFinite(dsp::lerp(sample, filtered, blend));
        }

    private:
        void updateIfNeeded(float shift) noexcept
        {
            if (std::abs(shift - cachedShift_) < 1.0e-4f)
                return;
            cachedShift_ = shift;

            const float amount = std::min(std::abs(shift), 1.0f);
            const float gainDb = amount * 12.0f;
            if (shift >= 0.0f)
            {
                lowShelf_.setLowShelf(900.0f, -gainDb * 0.45f, sampleRate_);
                highShelf_.setHighShelf(2200.0f, gainDb, sampleRate_);
            }
            else
            {
                lowShelf_.setLowShelf(900.0f, gainDb, sampleRate_);
                highShelf_.setHighShelf(2200.0f, -gainDb * 0.45f, sampleRate_);
            }
        }

        double sampleRate_ = 48000.0;
        float cachedShift_ = 999.0f;
        dsp::Biquad lowShelf_;
        dsp::Biquad highShelf_;
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
