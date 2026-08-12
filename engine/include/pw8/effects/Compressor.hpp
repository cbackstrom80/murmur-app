#pragma once

#include <algorithm>
#include <cmath>

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/OutputTransformerStage.hpp"

// A feedforward peak compressor with a soft knee -- the master spec's "first
// effect set" basic compressor (docs/ROADMAP.md "GATE 10"). Standard, openly
// documented topology: peak detector (stereo-linked -- a single shared envelope
// driven by max(|L|,|R|), so a loud transient in either channel ducks both
// equally rather than shifting the stereo image) -> a quadratic soft-knee gain
// computer (the standard formula: within +-knee/2 of the threshold, the gain
// reduction curve is a parabola blending smoothly from "no reduction" to "full
// ratio," rather than snapping at the threshold) -> asymmetric attack/release
// smoothing of the gain-reduction envelope itself (in dB, not linear, so the
// perceived speed is consistent across levels) -> optional post-GR output
// transformer colour -> makeup gain.
namespace pw8::effects
{
    class CompressorProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            transformer_.prepare(sampleRate);
            reset();
        }

        void reset() noexcept
        {
            gainReductionDb_ = 0.0f;
            transformer_.reset();
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float peak = std::max(std::abs(inL), std::abs(inR));
            const float peakDb = dsp::gainToDb(std::max(peak, 1.0e-6f));

            const float threshold = p.compThresholdDb;
            const float ratio = std::max(p.compRatio, 1.0f);
            const float knee = std::max(p.compKneeDb, 0.0f);
            const float overshoot = peakDb - threshold;

            float targetReductionDb;
            if (knee > 0.0f && std::abs(overshoot) < knee / 2.0f)
            {
                // Quadratic soft knee: blends smoothly across the knee width rather
                // than a hard corner at the threshold.
                const float x = overshoot + knee / 2.0f;
                targetReductionDb = (1.0f / ratio - 1.0f) * (x * x) / (2.0f * knee);
            }
            else if (overshoot > 0.0f)
            {
                targetReductionDb = (1.0f / ratio - 1.0f) * overshoot;
            }
            else
            {
                targetReductionDb = 0.0f;
            }

            // Attack when reduction needs to deepen (more negative), release when it recovers.
            const bool attacking = targetReductionDb < gainReductionDb_;
            const float timeMs = std::max(attacking ? p.compAttackMs : p.compReleaseMs, 0.01f);
            const float coeff = 1.0f - std::exp(-1.0f / (0.001f * timeMs * sr));
            gainReductionDb_ += (targetReductionDb - gainReductionDb_) * coeff;

            const float grGain = dsp::dbToGain(gainReductionDb_);
            float wetL = inL * grGain;
            float wetR = inR * grGain;

            transformer_.processStereo(wetL, wetR, p, gainReductionDb_);

            const float makeupGain = dsp::dbToGain(p.compMakeupDb);
            wetL *= makeupGain;
            wetR *= makeupGain;

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = dsp::lerp(inL, wetL, mix);
            outR = dsp::lerp(inR, wetR, mix);
        }

    private:
        double sampleRate_ = 48000.0;
        float gainReductionDb_ = 0.0f; // always <= 0 -- a running dB offset, smoothed toward the target each sample.
        OutputTransformerStage transformer_{};
    };

} // namespace pw8::effects
