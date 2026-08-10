#pragma once

#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/HilbertTransformer.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// A delay with a single-sideband frequency shifter placed *inside* the feedback
// loop, informed by ValhallaFreqEcho's research (docs/FX_BANK.md
// "ValhallaFreqEcho"): every repeat's frequency content is shifted by a further
// `freqShiftHz`, so a held chord's harmonics drift out of tune more with each
// echo -- rising/falling glissandos, barberpole phasing, and (at high feedback)
// self-oscillating runaway sweeps, all from the shift amount and feedback alone.
// Low/high-cut one-pole filters in the loop keep it from building either DC or
// harsh high-frequency energy indefinitely.
namespace pw8::effects
{
    class FreqShiftEchoProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            lineL_.prepare(sampleRate, kMaxEffectDelaySeconds);
            lineR_.prepare(sampleRate, kMaxEffectDelaySeconds);
            shifterL_.prepare(sampleRate);
            shifterR_.prepare(sampleRate);
            reset();
        }

        void reset() noexcept
        {
            lineL_.reset();
            lineR_.reset();
            shifterL_.reset();
            shifterR_.reset();
            hpStateL_ = hpStateR_ = lpStateL_ = lpStateR_ = 0.0f;
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float delaySamples =
                dsp::clamp(p.freqShiftDelayMs, 1.0f, kMaxEffectDelaySeconds * 1000.0f) * 0.001f * sr;

            const float tapL = lineL_.readInterpolated(delaySamples);
            const float tapR = lineR_.readInterpolated(delaySamples);

            const float filteredL = shapeFeedback(tapL, p, hpStateL_, lpStateL_, sr);
            const float filteredR = shapeFeedback(tapR, p, hpStateR_, lpStateR_, sr);

            const float shiftedL = shifterL_.process(filteredL, p.freqShiftHz);
            const float shiftedR = shifterR_.process(filteredR, p.freqShiftHz);

            const float feedback = dsp::clamp(p.freqShiftFeedback, 0.0f, 0.98f);
            lineL_.write(inL + shiftedL * feedback);
            lineR_.write(inR + shiftedR * feedback);

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = inL + shiftedL * mix;
            outR = inR + shiftedR * mix;
        }

    private:
        [[nodiscard]] static float shapeFeedback(float x, const EffectSlotParams& p, float& hpState, float& lpState,
                                                   float sr) noexcept
        {
            // One-pole highpass (input minus a lowpassed copy of itself) removes
            // rumble below `freqShiftLowCutHz`; a following one-pole lowpass tames
            // harshness above `freqShiftHighCutHz`. Cheap and unconditionally stable
            // -- enough to keep a frequency-shifting feedback loop well-behaved.
            const float hpCoeff = std::exp(-dsp::kTwoPi * dsp::clamp(p.freqShiftLowCutHz, 5.0f, sr * 0.49f) / sr);
            hpState = hpCoeff * hpState + (1.0f - hpCoeff) * x;
            const float highPassed = x - hpState;

            const float lpCoeff = std::exp(-dsp::kTwoPi * dsp::clamp(p.freqShiftHighCutHz, 20.0f, sr * 0.49f) / sr);
            lpState = lpCoeff * lpState + (1.0f - lpCoeff) * highPassed;
            return dsp::flushIfNotFinite(lpState);
        }

        double sampleRate_ = 48000.0;
        dsp::DelayLine lineL_;
        dsp::DelayLine lineR_;
        dsp::FrequencyShifter shifterL_;
        dsp::FrequencyShifter shifterR_;
        float hpStateL_ = 0.0f;
        float hpStateR_ = 0.0f;
        float lpStateL_ = 0.0f;
        float lpStateR_ = 0.0f;
    };

} // namespace pw8::effects
