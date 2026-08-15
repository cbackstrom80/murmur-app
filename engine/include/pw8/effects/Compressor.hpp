#pragma once

#include <algorithm>
#include <cmath>

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/OutputTransformerStage.hpp"

// Feedforward peak compressor with soft knee, optional auto-makeup, and VCA/FET/Opto
// character curves (FX deep pass — docs/FX_DEEP_PASS_PLAN.md §B).
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

        [[nodiscard]] float getGainReductionDb() const noexcept { return gainReductionDb_; }

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

            float attackMs = std::max(p.compAttackMs, 0.01f);
            float releaseMs = std::max(p.compReleaseMs, 0.01f);
            applyCharacterTiming(p.compCharacter, attackMs, releaseMs);

            const bool attacking = targetReductionDb < gainReductionDb_;
            const float timeMs = attacking ? attackMs : releaseMs;
            const float coeff = 1.0f - std::exp(-1.0f / (0.001f * timeMs * sr));
            gainReductionDb_ += (targetReductionDb - gainReductionDb_) * coeff;

            const float grGain = dsp::dbToGain(gainReductionDb_);
            float wetL = inL * grGain;
            float wetR = inR * grGain;

            transformer_.processStereo(wetL, wetR, p, gainReductionDb_);

            float makeupDb = p.compMakeupDb;
            if (p.compAutoMakeup && gainReductionDb_ < -0.01f)
            {
                const float estimated = -gainReductionDb_ * (1.0f - 1.0f / ratio) * 0.55f;
                makeupDb += estimated;
            }

            const float makeupGain = dsp::dbToGain(makeupDb);
            wetL *= makeupGain;
            wetR *= makeupGain;

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = dsp::lerp(inL, wetL, mix);
            outR = dsp::lerp(inR, wetR, mix);
        }

    private:
        static void applyCharacterTiming(int character, float& attackMs, float& releaseMs) noexcept
        {
            switch (static_cast<CompCharacter>(dsp::clamp(character, 0, 2)))
            {
                case CompCharacter::Fet:
                    attackMs = std::max(attackMs * 0.35f, 0.05f);
                    releaseMs = std::max(releaseMs * 0.55f, 5.0f);
                    break;
                case CompCharacter::Opto:
                    attackMs = std::max(attackMs * 2.2f, 1.0f);
                    releaseMs = std::max(releaseMs * 2.5f, 80.0f);
                    break;
                case CompCharacter::Vca:
                default:
                    break;
            }
        }

        double sampleRate_ = 48000.0;
        float gainReductionDb_ = 0.0f;
        OutputTransformerStage transformer_{};
    };

} // namespace pw8::effects
