#pragma once

#include <algorithm>
#include <cmath>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// Post–gain-reduction output-transformer colour for the master compressor.
// Macro model: GR-dependent drive, core-specific saturation/asymmetry, gentle
// HF rolloff, optional brand EQ bias (Jensen / Cinemag / Sowter).
namespace pw8::effects
{
    class OutputTransformerStage
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            reset();
            updateFilters();
        }

        void reset() noexcept
        {
            hfL_.reset();
            hfR_.reset();
            brandL_.reset();
            brandR_.reset();
        }

        void processStereo(float& l, float& r, const EffectSlotParams& p, float gainReductionDb) noexcept
        {
            const int core = static_cast<int>(std::lround(p.compTransformerCore));
            const float amount = dsp::clamp(p.compTransformerAmount, 0.0f, 1.0f);
            if (core <= 0 || amount <= 1.0e-4f)
                return;

            if (core != lastCore_ || static_cast<int>(std::lround(p.compTransformerBrand)) != lastBrand_)
            {
                lastCore_ = core;
                lastBrand_ = static_cast<int>(std::lround(p.compTransformerBrand));
                updateFilters();
            }

            const float grDrive = std::min(std::abs(gainReductionDb) * 0.035f * amount, 0.65f);
            l = shapeSample(l, core, grDrive);
            r = shapeSample(r, core, grDrive);
            l = hfL_.renderSample(l);
            r = hfR_.renderSample(r);
            if (lastBrand_ > 0)
            {
                l = brandL_.renderSample(l);
                r = brandR_.renderSample(r);
            }
        }

    private:
        [[nodiscard]] static float shapeSample(float x, int core, float grDrive) noexcept
        {
            float drive = 1.0f;
            float asym = 0.0f;
            float clipGain = 1.0f;
            switch (core)
            {
                case 1: // Nickel — soft, slightly bright
                    drive = 1.15f + grDrive * 1.4f;
                    clipGain = 0.92f;
                    break;
                case 2: // Iron — asymmetric even harmonics
                    drive = 1.35f + grDrive * 2.0f;
                    asym = 0.22f + grDrive * 0.35f;
                    clipGain = 0.88f;
                    break;
                case 3: // Steel — denser odd harmonics
                    drive = 1.55f + grDrive * 2.6f;
                    asym = 0.08f;
                    clipGain = 0.82f;
                    break;
                default: return x;
            }

            const float scaled = x * drive;
            const float even = asym * scaled * std::abs(scaled);
            const float shaped = std::tanh((scaled + even) * clipGain);
            return dsp::flushIfNotFinite(shaped);
        }

        void updateFilters() noexcept
        {
            float rolloffHz = 16000.0f;
            switch (lastCore_)
            {
                case 1: rolloffHz = 12000.0f; break;
                case 2: rolloffHz = 10000.0f; break;
                case 3: rolloffHz = 8500.0f; break;
                default: break;
            }

            hfL_.setLowpass(rolloffHz, sampleRate_);
            hfR_.setLowpass(rolloffHz, sampleRate_);

            brandL_.reset();
            brandR_.reset();
            switch (lastBrand_)
            {
                case 1: // Jensen — forward mid presence
                    brandL_.setPeaking(2200.0f, 1.2f, 0.9f, sampleRate_);
                    brandR_.setPeaking(2200.0f, 1.2f, 0.9f, sampleRate_);
                    break;
                case 2: // Cinemag — low warmth
                    brandL_.setLowShelf(120.0f, 1.4f, sampleRate_);
                    brandR_.setLowShelf(120.0f, 1.4f, sampleRate_);
                    break;
                case 3: // Sowter — darker top
                    brandL_.setHighShelf(6500.0f, -1.5f, sampleRate_);
                    brandR_.setHighShelf(6500.0f, -1.5f, sampleRate_);
                    break;
                default: break;
            }
        }

        double sampleRate_ = 48000.0;
        int lastCore_ = -1;
        int lastBrand_ = -1;
        dsp::Biquad hfL_{};
        dsp::Biquad hfR_{};
        dsp::Biquad brandL_{};
        dsp::Biquad brandR_{};
    };

} // namespace pw8::effects
