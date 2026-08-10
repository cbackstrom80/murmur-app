#pragma once

#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// A modulated feedforward delay per channel, the two channels' LFOs held 90
// degrees apart for stereo width -- the standard chorus technique. See
// docs/FX_BANK.md.
namespace pw8::effects
{
    class ChorusProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            delayL_.prepare(sampleRate, kMaxDelaySeconds);
            delayR_.prepare(sampleRate, kMaxDelaySeconds);
            phase_ = 0.0f;
        }

        void reset() noexcept
        {
            delayL_.reset();
            delayR_.reset();
            phase_ = 0.0f;
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            phase_ = dsp::wrapPhase(phase_ + p.chorusRateHz / sr);

            const float lfoL = std::sin(dsp::kTwoPi * phase_);
            const float lfoR = std::sin(dsp::kTwoPi * dsp::wrapPhase(phase_ + 0.25f));

            const float baseSamples = dsp::clamp(p.chorusBaseDelayMs, 1.0f, kMaxDelaySeconds * 1000.0f) * 0.001f * sr;
            const float depthSamples = dsp::clamp(p.chorusDepthMs, 0.0f, kMaxDelaySeconds * 500.0f) * 0.001f * sr;

            // Read before write: chorus is a feedforward (not feedback) comb, so the
            // wet tap is purely a function of past input, never today's sample.
            const float wetL = delayL_.readInterpolated(dsp::clamp(baseSamples + depthSamples * lfoL, 1.0f,
                                                                     sr * kMaxDelaySeconds - 4.0f));
            const float wetR = delayR_.readInterpolated(dsp::clamp(baseSamples + depthSamples * lfoR, 1.0f,
                                                                     sr * kMaxDelaySeconds - 4.0f));
            delayL_.write(inL);
            delayR_.write(inR);

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = dsp::lerp(inL, wetL, mix);
            outR = dsp::lerp(inR, wetR, mix);
        }

    private:
        static constexpr float kMaxDelaySeconds = 0.1f; ///< base delay + depth never exceeds ~60ms in practice.

        double sampleRate_ = 48000.0;
        dsp::DelayLine delayL_;
        dsp::DelayLine delayR_;
        float phase_ = 0.0f;
    };

} // namespace pw8::effects
