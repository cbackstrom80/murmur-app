#pragma once

#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/TempoSync.hpp"
#include "pw8/effects/EffectTypes.hpp"

// Warm analog/tape-style stereo delay, informed by Cocoa Delay's feature set
// (docs/FX_BANK.md "Cocoa Delay"): wow/flutter time drift, drive-based
// saturation on the feedback path, sidechain-style ducking under the dry
// signal, and three pan behaviors (Static independent channels, PingPong
// cross-feedback bounce, Circular continuously-rotating stereo image).
namespace pw8::effects
{
    class TapeDelayProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            lineA_.prepare(sampleRate, kMaxEffectDelaySeconds);
            lineB_.prepare(sampleRate, kMaxEffectDelaySeconds);
            reset();
        }

        void reset() noexcept
        {
            lineA_.reset();
            lineB_.reset();
            duckEnvelope_ = 0.0f;
            driftPhase_ = 0.0f;
            circularPhase_ = 0.0f;
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR,
                           float bpm = 120.0f) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);

            // Shared wow/flutter LFO -- real tape drifts both channels together.
            driftPhase_ = dsp::wrapPhase(driftPhase_ + p.tapeDriftRateHz / sr);
            const float driftSamples = std::sin(dsp::kTwoPi * driftPhase_) * p.tapeDriftDepthMs * 0.001f * sr;

            const float delayMs = dsp::effectiveDelayMs(p.tapeDelaySync, p.tapeDelaySyncDivisionIndex, p.tapeDelayMs,
                                                        bpm, 1.0f, kMaxEffectDelaySeconds * 1000.0f);
            const float baseSamples = delayMs * 0.001f * sr;
            const float delaySamples =
                dsp::clamp(baseSamples + driftSamples, 1.0f, sr * kMaxEffectDelaySeconds - 4.0f);

            const float driveLinear = dsp::dbToGain(p.tapeDriveDb);
            const float feedback = dsp::clamp(p.tapeFeedback, 0.0f, 0.98f);

            float wetL, wetR;

            if (p.tapePanMode == DelayPanMode::PingPong)
            {
                // Cross-feedback: A receives fresh input plus B's decayed tail, B
                // receives only A's decayed tail -- the classic alternating bounce.
                const float monoIn = (inL + inR) * 0.5f;
                const float tapA = lineA_.readInterpolated(delaySamples);
                const float tapB = lineB_.readInterpolated(delaySamples);
                const float satA = dsp::softSaturate(tapA, driveLinear);
                const float satB = dsp::softSaturate(tapB, driveLinear);
                lineA_.write(monoIn + satB * feedback);
                lineB_.write(satA * feedback);
                wetL = tapA;
                wetR = tapB;
            }
            else
            {
                const float tapL = lineA_.readInterpolated(delaySamples);
                const float tapR = lineB_.readInterpolated(delaySamples);
                const float satL = dsp::softSaturate(tapL, driveLinear);
                const float satR = dsp::softSaturate(tapR, driveLinear);
                lineA_.write(inL + satL * feedback);
                lineB_.write(inR + satR * feedback);
                wetL = tapL;
                wetR = tapR;
            }

            if (p.tapePanMode == DelayPanMode::Circular)
            {
                circularPhase_ = dsp::wrapPhase(circularPhase_ + (p.tapeDriftRateHz * 2.0f + 0.05f) / sr);
                const float monoWet = (wetL + wetR) * 0.5f;
                const float panPos = (std::sin(dsp::kTwoPi * circularPhase_) + 1.0f) * 0.5f; // 0..1
                const float halfPi = dsp::kPi * 0.5f;
                wetL = monoWet * std::cos(panPos * halfPi);
                wetR = monoWet * std::sin(panPos * halfPi);
            }

            // Ducking: a slow envelope follower on the dry signal attenuates the wet
            // mix while the dry input is loud, sidechain-style, so repeats don't
            // clutter under a busy passage. Fixed-rate follower (documented, not yet
            // an exposed parameter) -- see docs/FX_BANK.md "What's not exposed yet".
            const float dryLevel = std::abs(inL) + std::abs(inR);
            constexpr float kDuckFollowerCoeff = 0.001f;
            duckEnvelope_ += (dryLevel - duckEnvelope_) * kDuckFollowerCoeff;
            const float duckGain =
                1.0f - dsp::clamp(p.tapeDuckAmount, 0.0f, 1.0f) * dsp::clamp(duckEnvelope_ * 2.0f, 0.0f, 1.0f);

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f) * duckGain;
            outL = inL + wetL * mix;
            outR = inR + wetR * mix;
        }

    private:
        double sampleRate_ = 48000.0;
        dsp::DelayLine lineA_;
        dsp::DelayLine lineB_;
        float duckEnvelope_ = 0.0f;
        float driftPhase_ = 0.0f;
        float circularPhase_ = 0.0f;
    };

} // namespace pw8::effects
