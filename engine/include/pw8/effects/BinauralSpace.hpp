#pragma once

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/BinauralPanner.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/RoomEngine.hpp"

// QUASAR master-bus binaural spatial mixer — Phase 2 (see docs/GLOBAL_QUASAR_FX_PLAN.md).
// Dual QSR paths (ITD/ILD + embedded room) + CNTR dry anchor + post-sum stereo delay.
namespace pw8::effects
{
    class BinauralSpaceProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            pannerQsr1_.prepare(sampleRate);
            pannerQsr2_.prepare(sampleRate);
            roomQsr1_.prepare(sampleRate);
            roomQsr2_.prepare(sampleRate);
            qsrHpfL_.reset();
            qsrHpfR_.reset();
            cntrHpfL_.reset();
            cntrHpfR_.reset();
            delayL_.prepare(sampleRate, kMaxQuasarDelaySeconds);
            delayR_.prepare(sampleRate, kMaxQuasarDelaySeconds);
            delayHpfL_.reset();
            delayHpfR_.reset();
            delayLpfL_.reset();
            delayLpfR_.reset();
        }

        void reset() noexcept
        {
            pannerQsr1_.reset();
            pannerQsr2_.reset();
            roomQsr1_.reset();
            roomQsr2_.reset();
            qsrHpfL_.reset();
            qsrHpfR_.reset();
            cntrHpfL_.reset();
            cntrHpfR_.reset();
            delayL_.reset();
            delayR_.reset();
            delayHpfL_.reset();
            delayHpfR_.reset();
            delayLpfL_.reset();
            delayLpfR_.reset();
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            if (mix <= 1.0e-6f)
            {
                outL = inL;
                outR = inR;
                return;
            }

            const float sr = static_cast<float>(sampleRate_);

            // Input conditioning HPFs.
            const float qsrHpfHz = dsp::clamp(p.inputSplitHpfHz, 20.0f, 500.0f);
            const float cntrHpfHz = dsp::clamp(p.cntrHpfHz, 20.0f, 300.0f);
            qsrHpfL_.setHighpass(qsrHpfHz, sampleRate_);
            qsrHpfR_.setHighpass(qsrHpfHz, sampleRate_);
            cntrHpfL_.setHighpass(cntrHpfHz, sampleRate_);
            cntrHpfR_.setHighpass(cntrHpfHz, sampleRate_);

            const float qsrMono = (qsrHpfL_.renderSample(inL) + qsrHpfR_.renderSample(inR)) * 0.5f;
            const float cntrL = cntrHpfL_.renderSample(inL);
            const float cntrR = cntrHpfR_.renderSample(inR);

            // QSR1 path.
            SpatialParams sp1{p.qsr1AngleDeg, p.qsr1Height, p.qsr1Distance};
            float q1L = 0.0f, q1R = 0.0f;
            pannerQsr1_.processMono(qsrMono, sp1, q1L, q1R);
            RoomParams rp1{p.qsr1RoomSize, p.qsr1RoomDamping, p.qsr1RoomAmount};
            float r1L = 0.0f, r1R = 0.0f;
            roomQsr1_.processMono(qsrMono, rp1, r1L, r1R);
            q1L += r1L;
            q1R += r1R;

            // QSR2 path.
            SpatialParams sp2{p.qsr2AngleDeg, p.qsr2Height, p.qsr2Distance};
            float q2L = 0.0f, q2R = 0.0f;
            pannerQsr2_.processMono(qsrMono, sp2, q2L, q2R);
            RoomParams rp2{p.qsr2RoomSize, p.qsr2RoomDamping, p.qsr2RoomAmount};
            float r2L = 0.0f, r2R = 0.0f;
            roomQsr2_.processMono(qsrMono, rp2, r2L, r2R);
            q2L += r2L;
            q2R += r2R;

            // Sum QSR + CNTR.
            const float qsr1Lvl = dsp::clamp(p.qsr1Level, 0.0f, 1.0f);
            const float qsr2Lvl = dsp::clamp(p.qsr2Level, 0.0f, 1.0f);
            const float cntrLvl = dsp::clamp(p.cntrLevel, 0.0f, 1.0f);

            float wetL = q1L * qsr1Lvl + q2L * qsr2Lvl + cntrL * cntrLvl;
            float wetR = q1R * qsr1Lvl + q2R * qsr2Lvl + cntrR * cntrLvl;

            // Post-sum stereo delay (basic tape-style cross-feedback).
            const float delayMs = dsp::clamp(p.quasarDelayTimeMs, 3.0f, 20000.0f);
            const float delaySamples = dsp::clamp(delayMs * 0.001f * sr, 1.0f, sr * kMaxQuasarDelaySeconds - 4.0f);
            const float feedback = dsp::clamp(p.quasarDelayFeedback, 0.0f, 1.0f);
            const float delayVol = dsp::clamp(p.quasarDelayVolume, 0.0f, 1.0f);

            const float dInL = wetL;
            const float dInR = wetR;
            const float tapL = delayL_.readInterpolated(delaySamples);
            const float tapR = delayR_.readInterpolated(delaySamples);

            delayL_.write(dInL + tapR * feedback);
            delayR_.write(dInR + tapL * feedback);

            const float delayHpfHz = 180.0f;
            const float delayLpfHz = 4200.0f;
            delayHpfL_.setHighpass(delayHpfHz, sampleRate_);
            delayHpfR_.setHighpass(delayHpfHz, sampleRate_);
            delayLpfL_.setLowpass(delayLpfHz, sampleRate_);
            delayLpfR_.setLowpass(delayLpfHz, sampleRate_);

            const float dL = delayLpfL_.renderSample(delayHpfL_.renderSample(tapL)) * delayVol;
            const float dR = delayLpfR_.renderSample(delayHpfR_.renderSample(tapR)) * delayVol;
            wetL += dL;
            wetR += dR;

            outL = inL + (wetL - inL) * mix;
            outR = inR + (wetR - inR) * mix;
        }

        [[nodiscard]] static int latencySamples(double sampleRate, float delayTimeMs) noexcept
        {
            const int itd = BinauralPanner::maxItdLatencySamples(sampleRate);
            const int delay = static_cast<int>(std::ceil(dsp::clamp(delayTimeMs, 3.0f, 20000.0f) * 0.001f * sampleRate));
            return itd + delay;
        }

    private:
        double sampleRate_ = 48000.0;
        BinauralPanner pannerQsr1_{};
        BinauralPanner pannerQsr2_{};
        RoomEngine roomQsr1_{};
        RoomEngine roomQsr2_{};
        dsp::Biquad qsrHpfL_{};
        dsp::Biquad qsrHpfR_{};
        dsp::Biquad cntrHpfL_{};
        dsp::Biquad cntrHpfR_{};
        dsp::DelayLine delayL_{};
        dsp::DelayLine delayR_{};
        dsp::Biquad delayHpfL_{};
        dsp::Biquad delayHpfR_{};
        dsp::Biquad delayLpfL_{};
        dsp::Biquad delayLpfR_{};
    };

} // namespace pw8::effects
