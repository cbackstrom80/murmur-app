#pragma once

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/TempoSync.hpp"
#include "pw8/effects/BinauralPanner.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/RoomEngine.hpp"

// QUASAR binaural spatial mixer (see docs/QUASAR_RETURN_PLAN.md, docs/GLOBAL_QUASAR_FX_PLAN.md).
// Master-bus EffectType::BinauralSpace in MURMUR; legacy reference in quasar_plugin/.
namespace pw8::effects
{
    inline constexpr float kMaxQuasarDelaySeconds = 20.0f;

    struct BinauralSpaceParams
    {
        float mix = 1.0f;
        float qsr1Level = 0.65f;
        float qsr2Level = 0.55f;
        float cntrLevel = 0.85f;
        float inputSplitHpfHz = 120.0f;
        float cntrHpfHz = 80.0f;
        float qsr1Height = 0.0f;
        float qsr1AngleDeg = 30.0f;
        float qsr1Distance = 0.35f;
        float qsr2Height = 0.0f;
        float qsr2AngleDeg = 330.0f;
        float qsr2Distance = 0.4f;
        float qsr1RoomAmount = 0.45f;
        float qsr1RoomSize = 1.0f;
        float qsr1RoomDamping = 0.55f;
        float qsr2RoomAmount = 0.40f;
        float qsr2RoomSize = 1.1f;
        float qsr2RoomDamping = 0.50f;
        float quasarDelayTimeMs = 450.0f;
        float quasarDelayFeedback = 0.35f;
        float quasarDelayVolume = 0.25f;
        /// 0 = Headphone, 1 = Speaker, 2 = Auto (default Headphone).
        int quasarOutputMode = 0;
        float quasarCrossfeed = 0.0f;
        bool quasarDelaySync = false;
        int quasarDelaySyncDivisionIndex = static_cast<int>(dsp::kDefaultDelaySyncDivisionIndex);
        /// When true, left input → QSR1 path, right input → QSR2 path (Quasar PLAY default).
        /// When false, summed mono feeds both QSR paths (legacy MURMUR master FX behavior).
        bool qsrStereoSplit = true;
    };

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

        void processStereo(float inL, float inR, const BinauralSpaceParams& p, float& outL, float& outR,
                           float bpm = 120.0f, float qsr2AuxIn = 0.0f, bool useQsr2Aux = false) noexcept
        {
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            if (mix <= 1.0e-6f)
            {
                outL = inL;
                outR = inR;
                return;
            }

            // Headphone vs Speaker compensation (Phase 3).
            const int outputMode = p.quasarOutputMode;
            const bool speakerMode = outputMode == 1;
            const float itdScale = speakerMode ? 0.3f : 1.0f;
            const float crossfeedAmt =
                speakerMode ? dsp::clamp(dsp::lerp(0.25f, 0.4f, p.quasarCrossfeed), 0.0f, 0.5f) : 0.0f;

            const float sr = static_cast<float>(sampleRate_);

            // Input conditioning HPFs.
            const float qsrHpfHz = dsp::clamp(p.inputSplitHpfHz, 20.0f, 500.0f);
            const float cntrHpfHz = dsp::clamp(p.cntrHpfHz, 20.0f, 300.0f);
            qsrHpfL_.setHighpass(qsrHpfHz, sampleRate_);
            qsrHpfR_.setHighpass(qsrHpfHz, sampleRate_);
            cntrHpfL_.setHighpass(cntrHpfHz, sampleRate_);
            cntrHpfR_.setHighpass(cntrHpfHz, sampleRate_);

            const float qsrLeftMono = qsrHpfL_.renderSample(inL);
            const float qsr2Source = useQsr2Aux ? qsr2AuxIn : inR;
            const float qsrRightMono = qsrHpfR_.renderSample(qsr2Source);
            const float qsrMonoSum = (qsrLeftMono + qsrRightMono) * 0.5f;
            const float qsr1Mono = p.qsrStereoSplit ? qsrLeftMono : qsrMonoSum;
            const float qsr2Mono = p.qsrStereoSplit ? qsrRightMono : qsrMonoSum;
            const float cntrL = cntrHpfL_.renderSample(inL);
            const float cntrR = cntrHpfR_.renderSample(inR);

            // QSR1 path.
            SpatialParams sp1{p.qsr1AngleDeg, p.qsr1Height, p.qsr1Distance};
            float q1L = 0.0f, q1R = 0.0f;
            pannerQsr1_.processMono(qsr1Mono, sp1, q1L, q1R, itdScale);
            RoomParams rp1{p.qsr1RoomSize, p.qsr1RoomDamping, p.qsr1RoomAmount};
            float r1L = 0.0f, r1R = 0.0f;
            roomQsr1_.processMono(qsr1Mono, rp1, r1L, r1R);
            q1L += r1L;
            q1R += r1R;

            // QSR2 path.
            SpatialParams sp2{p.qsr2AngleDeg, p.qsr2Height, p.qsr2Distance};
            float q2L = 0.0f, q2R = 0.0f;
            pannerQsr2_.processMono(qsr2Mono, sp2, q2L, q2R, itdScale);
            RoomParams rp2{p.qsr2RoomSize, p.qsr2RoomDamping, p.qsr2RoomAmount};
            float r2L = 0.0f, r2R = 0.0f;
            roomQsr2_.processMono(qsr2Mono, rp2, r2L, r2R);
            q2L += r2L;
            q2R += r2R;

            // Sum QSR + CNTR.
            const float qsr1Lvl = dsp::clamp(p.qsr1Level, 0.0f, 1.0f);
            const float qsr2Lvl = dsp::clamp(p.qsr2Level, 0.0f, 1.0f);
            const float cntrLvl = dsp::clamp(p.cntrLevel, 0.0f, 1.0f);

            float wetL = q1L * qsr1Lvl + q2L * qsr2Lvl + cntrL * cntrLvl;
            float wetR = q1R * qsr1Lvl + q2R * qsr2Lvl + cntrR * cntrLvl;

            if (crossfeedAmt > 1.0e-6f)
            {
                const float cfL = wetL * (1.0f - crossfeedAmt) + wetR * crossfeedAmt;
                const float cfR = wetR * (1.0f - crossfeedAmt) + wetL * crossfeedAmt;
                wetL = cfL;
                wetR = cfR;
            }

            // Post-sum stereo delay (Quasar FW 2.0 — freeze at feedback >= 0.99).
            const float delayMs = dsp::effectiveDelayMs(p.quasarDelaySync, p.quasarDelaySyncDivisionIndex,
                                                        p.quasarDelayTimeMs, bpm, 3.0f, 20000.0f);
            const float delaySamples = dsp::clamp(delayMs * 0.001f * sr, 1.0f, sr * kMaxQuasarDelaySeconds - 4.0f);
            const float feedback = dsp::clamp(p.quasarDelayFeedback, 0.0f, 1.0f);
            const float delayVol = dsp::clamp(p.quasarDelayVolume, 0.0f, 1.0f);
            const bool freeze = feedback >= 0.99f;
            const float fbGain = freeze ? 1.0f : feedback;

            const float dInL = wetL;
            const float dInR = wetR;
            const float tapL = delayL_.readInterpolated(delaySamples);
            const float tapR = delayR_.readInterpolated(delaySamples);

            if (freeze)
            {
                delayL_.write(tapL);
                delayR_.write(tapR);
            }
            else
            {
                delayL_.write(dInL + tapR * fbGain);
                delayR_.write(dInR + tapL * fbGain);
            }

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

    [[nodiscard]] inline BinauralSpaceParams binauralParamsFromEffectSlot(const EffectSlotParams& e) noexcept
    {
        BinauralSpaceParams p;
        p.mix = e.mix;
        p.qsr1Level = e.qsr1Level;
        p.qsr2Level = e.qsr2Level;
        p.cntrLevel = e.cntrLevel;
        p.inputSplitHpfHz = e.inputSplitHpfHz;
        p.cntrHpfHz = e.cntrHpfHz;
        p.qsr1Height = e.qsr1Height;
        p.qsr1AngleDeg = e.qsr1AngleDeg;
        p.qsr1Distance = e.qsr1Distance;
        p.qsr2Height = e.qsr2Height;
        p.qsr2AngleDeg = e.qsr2AngleDeg;
        p.qsr2Distance = e.qsr2Distance;
        p.qsr1RoomAmount = e.qsr1RoomAmount;
        p.qsr1RoomSize = e.qsr1RoomSize;
        p.qsr1RoomDamping = e.qsr1RoomDamping;
        p.qsr2RoomAmount = e.qsr2RoomAmount;
        p.qsr2RoomSize = e.qsr2RoomSize;
        p.qsr2RoomDamping = e.qsr2RoomDamping;
        p.quasarDelayTimeMs = e.quasarDelayTimeMs;
        p.quasarDelayFeedback = e.quasarDelayFeedback;
        p.quasarDelayVolume = e.quasarDelayVolume;
        p.quasarOutputMode = e.quasarOutputMode;
        p.quasarCrossfeed = e.quasarCrossfeed;
        p.quasarDelaySync = e.quasarDelaySync;
        p.quasarDelaySyncDivisionIndex = e.quasarDelaySyncDivisionIndex;
        p.qsrStereoSplit = e.qsrStereoSplit;
        return p;
    }

} // namespace pw8::effects
