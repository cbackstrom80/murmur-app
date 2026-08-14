#pragma once

#include <cmath>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"

// ITD/ILD binaural panner for Quasar QSR paths (Normal tier — see
// docs/GLOBAL_QUASAR_FX_PLAN.md §2.5). Maps azimuth/height/distance to
// interaural delay, level, and simple head-shadow filtering.
namespace pw8::effects
{
    struct SpatialParams
    {
        float angleDeg = 0.0f;   ///< 0..360 azimuth (0 = front).
        float height = 0.0f;       ///< -1..+1 elevation cue.
        float distance = 0.35f;    ///< 0..1 (0 = 20 cm, 1 = 10 m).
    };

    class BinauralPanner
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            itdDelayL_.prepare(sampleRate, kMaxItdSeconds);
            itdDelayR_.prepare(sampleRate, kMaxItdSeconds);
            shadowL_.reset();
            shadowR_.reset();
            airShelfL_.reset();
            airShelfR_.reset();
            elevationTiltL_.reset();
            elevationTiltR_.reset();
        }

        void reset() noexcept
        {
            itdDelayL_.reset();
            itdDelayR_.reset();
            shadowL_.reset();
            shadowR_.reset();
            airShelfL_.reset();
            airShelfR_.reset();
            elevationTiltL_.reset();
            elevationTiltR_.reset();
        }

        void processMono(float in, const SpatialParams& p, float& outL, float& outR,
                         float itdScale = 1.0f) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float angleRad = p.angleDeg * dsp::kPi / 180.0f;
            const float dist = dsp::clamp(p.distance, 0.0f, 1.0f);
            const float height = dsp::clamp(p.height, -1.0f, 1.0f);

            // ITD: sin/azimuth law, max ±0.8 ms (plan §2.5); scaled for Speaker mode.
            const float itdMs = 0.8f * std::sin(angleRad) * itdScale;
            const float heightItdMs = height * 0.15f * itdScale;
            const float itdLeftMs = std::max(-itdMs + heightItdMs, -0.8f);
            const float itdRightMs = std::max(itdMs - heightItdMs, -0.8f);

            const float delaySamplesL = dsp::clamp((0.8f + itdLeftMs) * 0.001f * sr, 1.0f,
                                                    sr * kMaxItdSeconds - 4.0f);
            const float delaySamplesR = dsp::clamp((0.8f + itdRightMs) * 0.001f * sr, 1.0f,
                                                    sr * kMaxItdSeconds - 4.0f);

            itdDelayL_.write(in);
            itdDelayR_.write(in);
            float left = itdDelayL_.readInterpolated(delaySamplesL);
            float right = itdDelayR_.readInterpolated(delaySamplesR);

            // ILD: equal-power pan with head-shadow attenuation on far ear.
            const float pan = std::sin(angleRad);
            const float ildL = std::sqrt(0.5f * (1.0f - pan));
            const float ildR = std::sqrt(0.5f * (1.0f + pan));
            left *= ildL;
            right *= ildR;

            // Head shadow: lowpass on contralateral ear (stronger when panned hard).
            const float shadowAmount = std::abs(pan);
            const float shadowHz = dsp::lerp(8000.0f, 1200.0f, shadowAmount);
            shadowL_.setLowpass(shadowHz, sampleRate_);
            shadowR_.setLowpass(dsp::lerp(8000.0f, 1200.0f, 1.0f - shadowAmount * 0.5f), sampleRate_);
            left = shadowL_.renderSample(left);
            right = shadowR_.renderSample(right);

            // Distance: log gain + air absorption shelf above 1 kHz.
            const float distGain = dsp::lerp(1.0f, 0.18f, dist * dist);
            const float airDb = dsp::lerp(0.0f, -9.0f, dist);
            airShelfL_.setHighShelf(1000.0f, airDb, sampleRate_);
            airShelfR_.setHighShelf(1000.0f, airDb, sampleRate_);
            left = airShelfL_.renderSample(left) * distGain;
            right = airShelfR_.renderSample(right) * distGain;

            // Height: spectral tilt + subtle contralateral elevation cue (HRIR-lite).
            const float tiltDb = height * 5.5f;
            const float contralateralTilt = height * 1.2f;
            elevationTiltL_.setHighShelf(2800.0f, tiltDb, sampleRate_);
            elevationTiltR_.setHighShelf(2800.0f, tiltDb * 0.82f - contralateralTilt, sampleRate_);
            left = elevationTiltL_.renderSample(left);
            right = elevationTiltR_.renderSample(right);

            outL = left;
            outR = right;
        }

        /// Peak ITD delay in samples at max azimuth — for latency reporting.
        [[nodiscard]] static int maxItdLatencySamples(double sampleRate) noexcept
        {
            return static_cast<int>(std::ceil(0.0016 * sampleRate)); // 1.6 ms round-trip buffer.
        }

    private:
        static constexpr float kMaxItdSeconds = 0.002f;

        double sampleRate_ = 48000.0;
        dsp::DelayLine itdDelayL_{};
        dsp::DelayLine itdDelayR_{};
        dsp::Biquad shadowL_{};
        dsp::Biquad shadowR_{};
        dsp::Biquad airShelfL_{};
        dsp::Biquad airShelfR_{};
        dsp::Biquad elevationTiltL_{};
        dsp::Biquad elevationTiltR_{};
    };

} // namespace pw8::effects
