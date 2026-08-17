#pragma once

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// A 3-band parametric EQ: low shelf, mid peak/bell, high shelf -- the master
// spec's "first effect set" basic EQ (docs/ROADMAP.md "GATE 10"). Each band is a
// `dsp::Biquad` using the RBJ Audio EQ Cookbook formulas. Coefficients are
// recomputed every sample (a handful of trig/pow calls) rather than cached and
// only refreshed on parameter change -- the same choice `filter::StateVariableFilter`
// already makes for Filter 1's cutoff/resonance, for the same reason: it stays
// glitch-free under live modulation/automation with no separate "did anything
// change" bookkeeping, at a cost this codebase has already judged acceptable
// relative to everything else in the per-sample signal path.
namespace pw8::effects
{
    class EqProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            reset();
        }

        void reset() noexcept
        {
            lowL_.reset();
            lowR_.reset();
            midL_.reset();
            midR_.reset();
            highL_.reset();
            highR_.reset();
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float lowFreq = dsp::clamp(p.eqLowFreqHz, 20.0f, sr * 0.45f);
            const float midFreq = dsp::clamp(p.eqMidFreqHz, 20.0f, sr * 0.45f);
            const float highFreq = dsp::clamp(p.eqHighFreqHz, 20.0f, sr * 0.45f);

            lowL_.setLowShelf(lowFreq, p.eqLowGainDb, sampleRate_);
            lowR_.setLowShelf(lowFreq, p.eqLowGainDb, sampleRate_);
            midL_.setPeaking(midFreq, p.eqMidGainDb, p.eqMidQ, sampleRate_);
            midR_.setPeaking(midFreq, p.eqMidGainDb, p.eqMidQ, sampleRate_);
            highL_.setHighShelf(highFreq, p.eqHighGainDb, sampleRate_);
            highR_.setHighShelf(highFreq, p.eqHighGainDb, sampleRate_);

            const float wetL = highL_.renderSample(midL_.renderSample(lowL_.renderSample(inL)));
            const float wetR = highR_.renderSample(midR_.renderSample(lowR_.renderSample(inR)));

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            const float outGain = dsp::dbToGain(dsp::clamp(p.eqOutGainDb, -12.0f, 12.0f));
            outL = dsp::lerp(inL, wetL, mix) * outGain;
            outR = dsp::lerp(inR, wetR, mix) * outGain;
        }

    private:
        double sampleRate_ = 48000.0;
        dsp::Biquad lowL_{}, lowR_{}, midL_{}, midR_{}, highL_{}, highR_{};
    };

} // namespace pw8::effects
