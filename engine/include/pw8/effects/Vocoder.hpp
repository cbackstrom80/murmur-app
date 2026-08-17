#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// Parallel filter-bank vocoder (8–16 bands). Carrier = insert input (layer voice sum);
// modulator = host sidechain when present. See docs/VOCODER_SIDECHAIN_PLAN.md.
namespace pw8::effects
{
    /// One log-spaced band: carrier + modulator bandpass, envelope follower on modulator.
    struct VocoderBand
    {
        dsp::Biquad carrierL{};
        dsp::Biquad carrierR{};
        dsp::Biquad modulator{};
        float envelope = 0.0f;

        void reset() noexcept
        {
            carrierL.reset();
            carrierR.reset();
            modulator.reset();
            clearEnvelope();
        }

        void clearEnvelope() noexcept { envelope = 0.0f; }

        void setBandpass(float centerHz, float q, double sampleRate) noexcept
        {
            carrierL.setBandpass(centerHz, q, sampleRate);
            carrierR.setBandpass(centerHz, q, sampleRate);
            modulator.setBandpass(centerHz, q, sampleRate);
        }

        void processSample(float carrierLIn, float carrierRIn, float modulatorIn, float attackAlpha,
                           float releaseAlpha, float& wetLOut, float& wetROut, float envScale) noexcept
        {
            const float carrierBandL = carrierL.renderSample(carrierLIn);
            const float carrierBandR = carrierR.renderSample(carrierRIn);
            const float modBand = modulator.renderSample(modulatorIn);
            const float rect = std::abs(modBand);
            const float alpha = rect > envelope ? attackAlpha : releaseAlpha;
            envelope = rect * alpha + envelope * (1.0f - alpha);
            envelope = dsp::flushIfNotFinite(envelope);
            const float env = envelope * envScale;
            wetLOut += carrierBandL * env;
            wetROut += carrierBandR * env;
        }
    };

    class VocoderProcessor
    {
    public:
        static constexpr int kMaxBands = 16;
        static constexpr float kMinCenterHz = 150.0f;
        static constexpr float kMaxCenterHz = 8000.0f;
        static constexpr float kDefaultAttackMs = 4.0f;
        static constexpr float kDefaultReleaseMs = 40.0f;

        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            reset();
            updateBandFilters(8, 1.0f);
            updateEnvelopeCoeffs(kDefaultReleaseMs);
        }

        void reset() noexcept
        {
            for (auto& band : bands_)
                band.reset();
            cachedBandCount_ = -1;
            cachedFormant_ = -1.0f;
            cachedReleaseMs_ = -1.0f;
        }

        void processStereo(float inL, float inR, float sidechainL, float sidechainR, const EffectSlotParams& p,
                           float& outL, float& outR, bool sidechainConnected = true) noexcept
        {
            if (!sidechainConnected)
            {
                const float inLSafe = dsp::flushIfNotFinite(inL);
                const float inRSafe = dsp::flushIfNotFinite(inR);
                outL = inLSafe;
                outR = inRSafe;
                return;
            }

            const int bandCount = dsp::clamp(p.vocoderBandCount, 8, kMaxBands);
            const float formantMul =
                std::pow(2.0f, (dsp::clamp(p.vocoderFormant, 0.0f, 1.0f) - 0.5f) * 2.0f);
            const float sibilance = dsp::clamp(p.vocoderSibilance, 0.0f, 1.0f);
            const float scGain = dsp::dbToGain(dsp::clamp(p.vocoderScGainDb, 0.0f, 48.0f));
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            const float releaseMs = dsp::clamp(p.vocoderReleaseMs, 5.0f, 250.0f);

            if (bandCount != cachedBandCount_ || std::abs(formantMul - cachedFormant_) > 1.0e-4f)
                updateBandFilters(bandCount, formantMul);
            if (std::abs(releaseMs - cachedReleaseMs_) > 0.05f)
                updateEnvelopeCoeffs(releaseMs);

            float modMono = (sidechainL + sidechainR) * 0.5f * scGain;
            modMono = dsp::flushIfNotFinite(modMono);

            const float inLSafe = dsp::flushIfNotFinite(inL);
            const float inRSafe = dsp::flushIfNotFinite(inR);

            float wetL = 0.0f;
            float wetR = 0.0f;
            for (int i = 0; i < bandCount; ++i)
            {
                const float sibilanceScale =
                    i >= (bandCount * 2) / 3 ? 1.0f + sibilance * 0.85f : 1.0f;
                bands_[static_cast<std::size_t>(i)].processSample(
                    inLSafe, inRSafe, modMono, attackAlpha_, releaseAlpha_, wetL, wetR, sibilanceScale);
            }

            wetL = dsp::flushIfNotFinite(wetL);
            wetR = dsp::flushIfNotFinite(wetR);
            wetL = std::tanh(wetL * 1.15f);
            wetR = std::tanh(wetR * 1.15f);

            outL = dsp::lerp(inLSafe, wetL, mix);
            outR = dsp::lerp(inRSafe, wetR, mix);
            outL = dsp::flushIfNotFinite(outL);
            outR = dsp::flushIfNotFinite(outR);
        }

    private:
        void updateBandFilters(int bandCount, float formantMul) noexcept
        {
            cachedBandCount_ = bandCount;
            cachedFormant_ = formantMul;
            const float logMin = std::log(kMinCenterHz);
            const float logMax = std::log(kMaxCenterHz);
            const float nyquistGuard = static_cast<float>(sampleRate_) * 0.45f;
            const float bandRatio =
                bandCount > 1 ? std::exp((logMax - logMin) / static_cast<float>(bandCount - 1)) : 1.0f;
            const float q = dsp::clamp(2.0f / (bandRatio - 1.0f), 2.5f, 10.0f);

            for (int i = 0; i < bandCount; ++i)
            {
                const float t =
                    bandCount > 1 ? static_cast<float>(i) / static_cast<float>(bandCount - 1) : 0.0f;
                const float centerHz = dsp::clamp(
                    std::exp(logMin + (logMax - logMin) * t) * formantMul, 40.0f, nyquistGuard);
                bands_[static_cast<std::size_t>(i)].setBandpass(centerHz, q, sampleRate_);
            }
        }

        void updateEnvelopeCoeffs(float releaseMs) noexcept
        {
            cachedReleaseMs_ = releaseMs;
            attackAlpha_ = alphaForMs(kDefaultAttackMs);
            releaseAlpha_ = alphaForMs(releaseMs);
        }

        [[nodiscard]] float alphaForMs(float ms) const noexcept
        {
            const float tau = ms * 0.001f * static_cast<float>(sampleRate_);
            return tau > 1.0f ? 1.0f - std::exp(-1.0f / tau) : 1.0f;
        }

        double sampleRate_ = 48000.0;
        int cachedBandCount_ = -1;
        float cachedFormant_ = -1.0f;
        float cachedReleaseMs_ = -1.0f;
        float attackAlpha_ = 0.5f;
        float releaseAlpha_ = 0.05f;
        std::array<VocoderBand, kMaxBands> bands_{};
    };

} // namespace pw8::effects
