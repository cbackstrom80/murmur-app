#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// 8–16 band channel vocoder MVP (DEEP CYCLE). Carrier = slot input (synth bus);
// modulator = AU sidechain audio when present, else mono sum of carrier (standalone
// self-test). Band envelopes follow the modulator; carrier bands are scaled and
// summed. Params reuse existing EffectSlotParams scalars when type == Vocoder —
// see docs/VOCODER_SIDECHAIN_PLAN.md.
namespace pw8::effects
{
    class VocoderProcessor
    {
    public:
        static constexpr int kMaxBands = 16;
        static constexpr float kMinCenterHz = 100.0f;
        static constexpr float kMaxCenterHz = 8000.0f;

        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            reset();
            updateBandFilters(8, 1.0f);
        }

        void reset() noexcept
        {
            for (auto& band : bands_)
            {
                band.carrierL.reset();
                band.carrierR.reset();
                band.modulator.reset();
                band.envelope = 0.0f;
            }
            cachedBandCount_ = -1;
            cachedFormant_ = -1.0f;
        }

        void processStereo(float inL, float inR, float sidechainL, float sidechainR, const EffectSlotParams& p,
                           float& outL, float& outR) noexcept
        {
            const int bandCount =
                dsp::clamp(static_cast<int>(p.freqShiftLowCutHz + 0.5f), 8, kMaxBands);
            const float formantMul = std::pow(2.0f, (dsp::clamp(p.fractalMorph, 0.0f, 1.0f) - 0.5f) * 2.0f);
            const float sibilance = dsp::clamp(p.freqShiftHz / 2000.0f, 0.0f, 1.0f);
            const float scGain = dsp::clamp(p.saturationDriveDb / 24.0f, 0.0f, 2.0f);
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);

            if (bandCount != cachedBandCount_ || std::abs(formantMul - cachedFormant_) > 1.0e-4f)
                updateBandFilters(bandCount, formantMul);

            float modMono = (sidechainL + sidechainR) * 0.5f * scGain;
            const bool sidechainSilent = std::abs(modMono) < 1.0e-6f;
            if (sidechainSilent)
                modMono = (inL + inR) * 0.5f;

            const float attackCoeff = coeffForMs(6.0f);
            const float releaseCoeff = coeffForMs(80.0f);

            float wetL = 0.0f;
            float wetR = 0.0f;
            for (int i = 0; i < bandCount; ++i)
            {
                auto& band = bands_[static_cast<std::size_t>(i)];
                const float carrierBandL = band.carrierL.renderSample(inL);
                const float carrierBandR = band.carrierR.renderSample(inR);
                const float modBand = band.modulator.renderSample(modMono);
                const float rect = std::abs(modBand);
                const float coeff = rect > band.envelope ? attackCoeff : releaseCoeff;
                band.envelope = rect + coeff * (band.envelope - rect);

                float env = band.envelope;
                if (i >= (bandCount * 2) / 3)
                    env *= 1.0f + sibilance * 0.85f;

                wetL += carrierBandL * env;
                wetR += carrierBandR * env;
            }

            const float norm = 1.0f / static_cast<float>(bandCount);
            wetL *= norm;
            wetR *= norm;

            outL = dsp::lerp(inL, wetL, mix);
            outR = dsp::lerp(inR, wetR, mix);
        }

    private:
        struct BandState
        {
            dsp::Biquad carrierL{};
            dsp::Biquad carrierR{};
            dsp::Biquad modulator{};
            float envelope = 0.0f;
        };

        void updateBandFilters(int bandCount, float formantMul) noexcept
        {
            cachedBandCount_ = bandCount;
            cachedFormant_ = formantMul;
            const float logMin = std::log(kMinCenterHz);
            const float logMax = std::log(kMaxCenterHz);

            for (int i = 0; i < bandCount; ++i)
            {
                const float t =
                    bandCount > 1 ? static_cast<float>(i) / static_cast<float>(bandCount - 1) : 0.0f;
                const float centerHz =
                    dsp::clamp(std::exp(logMin + (logMax - logMin) * t) * formantMul, 40.0f, 16000.0f);
                const float q = 4.5f;
                auto& band = bands_[static_cast<std::size_t>(i)];
                band.carrierL.setBandpass(centerHz, q, sampleRate_);
                band.carrierR.setBandpass(centerHz, q, sampleRate_);
                band.modulator.setBandpass(centerHz, q, sampleRate_);
            }
        }

        [[nodiscard]] float coeffForMs(float ms) const noexcept
        {
            const float tau = ms * 0.001f * static_cast<float>(sampleRate_);
            return tau > 1.0f ? std::exp(-1.0f / tau) : 0.0f;
        }

        double sampleRate_ = 48000.0;
        int cachedBandCount_ = -1;
        float cachedFormant_ = -1.0f;
        std::array<BandState, kMaxBands> bands_{};
    };

} // namespace pw8::effects
