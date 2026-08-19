#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/SidechainFollower.hpp"

// MIT clean-room master dynamics (Streams Track C — see docs/MASTER_DYNAMICS_SPEC.md).
namespace pw8::dynamics
{
    enum class MasterDynamicsMode : std::uint8_t
    {
        Envelope = 0,
        Vactrol,
        Follower,
        Compressor,
    };

    struct MasterDynamicsParams
    {
        bool enabled = false;
        MasterDynamicsMode mode = MasterDynamicsMode::Envelope;
        float thresholdDb = -12.0f;
        float ratio = 4.0f;
        float attackMs = 5.0f;
        float releaseMs = 80.0f;
        float sidechainGain = 1.0f;
        float vactrolSlewMs = 40.0f;
        float makeupDb = 0.0f;
        float mix = 1.0f;
    };

    class MasterDynamicsProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            sidechainFollower_.prepare(sampleRate_);
            reset();
        }

        void reset() noexcept
        {
            envelopeLevel_ = 0.0f;
            optoLevel_ = 0.0f;
            envelopeReleasing_ = true;
            optoReleasing_ = true;
            gainReductionDb_ = 0.0f;
            sidechainEnvelope_ = 0.0f;
            sidechainFollower_.reset();
        }

        void processSample(float& left, float& right, float sidechainLeft, float sidechainRight, bool triggerGate,
                           const MasterDynamicsParams& params, float mixModOffset = 0.0f,
                           float sidechainDepthMod = 0.0f) noexcept
        {
            if (!params.enabled)
            {
                gainReductionDb_ = 0.0f;
                sidechainEnvelope_ = 0.0f;
                return;
            }

            const float mix = dsp::clamp(params.mix + mixModOffset, 0.0f, 1.0f);
            if (mix <= 1.0e-6f)
            {
                gainReductionDb_ = 0.0f;
                return;
            }

            const float sidechainDepth = dsp::clamp(params.sidechainGain + sidechainDepthMod, 0.0f, 2.0f);
            float wetGain = 1.0f;

            switch (params.mode)
            {
                case MasterDynamicsMode::Envelope:
                    wetGain = processEnvelope(triggerGate, params.attackMs, params.releaseMs, envelopeLevel_,
                                              envelopeReleasing_);
                    gainReductionDb_ = wetGain < 0.999f ? dsp::gainToDb(wetGain) : 0.0f;
                    break;

                case MasterDynamicsMode::Vactrol:
                    wetGain = processVactrol(triggerGate, params.attackMs, params.vactrolSlewMs, optoLevel_,
                                             optoReleasing_);
                    gainReductionDb_ = wetGain < 0.999f ? dsp::gainToDb(wetGain) : 0.0f;
                    break;

                case MasterDynamicsMode::Follower:
                {
                    const float scMono =
                        std::sqrt((sidechainLeft * sidechainLeft + sidechainRight * sidechainRight) * 0.5f);
                    const float programPeak = std::max(std::abs(left), std::abs(right));
                    const float detector = std::max(scMono, programPeak * 0.35f);
                    sidechainEnvelope_ = followEnvelope(detector, params.attackMs, params.releaseMs);
                    wetGain = 1.0f - sidechainEnvelope_ * sidechainDepth;
                    wetGain = dsp::clamp(wetGain, 0.0f, 1.0f);
                    gainReductionDb_ = wetGain < 0.999f ? dsp::gainToDb(wetGain) : 0.0f;
                    break;
                }

                case MasterDynamicsMode::Compressor:
                {
                    const float scMono =
                        std::sqrt((sidechainLeft * sidechainLeft + sidechainRight * sidechainRight) * 0.5f);
                    const float peak = std::max(std::max(std::abs(left), std::abs(right)), scMono * sidechainDepth);
                    wetGain = processCompressor(peak, params);
                    gainReductionDb_ = wetGain < 0.999f ? dsp::gainToDb(wetGain) : 0.0f;
                    break;
                }
            }

            const float dryGain = 1.0f;
            const float gain = dsp::lerp(dryGain, wetGain, mix);
            left *= gain;
            right *= gain;
        }

        [[nodiscard]] float getGainReductionDb() const noexcept { return gainReductionDb_; }

        [[nodiscard]] float getSidechainEnvelope() const noexcept { return sidechainEnvelope_; }

    private:
        [[nodiscard]] float coeffForMs(float ms) const noexcept
        {
            const float tau = ms * 0.001f * static_cast<float>(sampleRate_);
            return tau > 1.0f ? std::exp(-1.0f / tau) : 0.0f;
        }

        [[nodiscard]] float followEnvelope(float input, float attackMs, float releaseMs) noexcept
        {
            const float x = dsp::flushIfNotFinite(input);
            const float attackCoeff = coeffForMs(std::max(attackMs, 0.1f));
            const float releaseCoeff = coeffForMs(std::max(releaseMs, 1.0f));
            const float coeff = x > sidechainEnvelope_ ? attackCoeff : releaseCoeff;
            sidechainEnvelope_ = x + coeff * (sidechainEnvelope_ - x);
            sidechainEnvelope_ = dsp::flushIfNotFinite(sidechainEnvelope_);
            return sidechainEnvelope_;
        }

        [[nodiscard]] float processEnvelope(bool trigger, float attackMs, float releaseMs, float& state,
                                            bool& releasing) noexcept
        {
            if (trigger)
            {
                state = 0.0f;
                releasing = false;
            }

            if (!releasing)
            {
                const float attackCoeff = 1.0f - coeffForMs(std::max(attackMs, 0.1f));
                state += (1.0f - state) * attackCoeff;
                if (state >= 0.99f)
                {
                    state = 1.0f;
                    releasing = true;
                }
            }
            else
            {
                const float releaseCoeff = coeffForMs(std::max(releaseMs, 1.0f));
                state *= releaseCoeff;
            }

            state = dsp::clamp(state, 0.0f, 1.0f);
            return state;
        }

        [[nodiscard]] float processVactrol(bool trigger, float attackMs, float slewMs, float& state,
                                           bool& releasing) noexcept
        {
            if (trigger)
            {
                state = 0.0f;
                releasing = false;
            }

            if (!releasing)
            {
                const float attackCoeff = 1.0f - coeffForMs(std::max(attackMs * 0.55f, 0.05f));
                state += (1.0f - state) * attackCoeff;
                if (state >= 0.97f)
                {
                    state = 1.0f;
                    releasing = true;
                }
            }
            else
            {
                const float releaseCoeff = coeffForMs(std::max(slewMs, 5.0f));
                state *= releaseCoeff;
            }

            state = dsp::clamp(state, 0.0f, 1.0f);
            return state;
        }

        [[nodiscard]] float processCompressor(float peak, const MasterDynamicsParams& params) noexcept
        {
            const float peakDb = dsp::gainToDb(std::max(peak, 1.0e-6f));
            const float threshold = params.thresholdDb;
            const float ratio = std::max(params.ratio, 1.0f);
            constexpr float kneeDb = 6.0f;
            const float overshoot = peakDb - threshold;

            float targetReductionDb = 0.0f;
            if (kneeDb > 0.0f && std::abs(overshoot) < kneeDb / 2.0f)
            {
                const float x = overshoot + kneeDb / 2.0f;
                targetReductionDb = (1.0f / ratio - 1.0f) * (x * x) / (2.0f * kneeDb);
            }
            else if (overshoot > 0.0f)
            {
                targetReductionDb = (1.0f / ratio - 1.0f) * overshoot;
            }

            const bool attacking = targetReductionDb < gainReductionDb_;
            const float timeMs =
                attacking ? std::max(params.attackMs, 0.1f) : std::max(params.releaseMs, 1.0f);
            const float coeff = 1.0f - coeffForMs(timeMs);
            gainReductionDb_ += (targetReductionDb - gainReductionDb_) * coeff;

            float wetGain = dsp::dbToGain(gainReductionDb_);
            wetGain *= dsp::dbToGain(params.makeupDb);
            return dsp::clamp(wetGain, 0.0f, 4.0f);
        }

        double sampleRate_ = 48000.0;
        float envelopeLevel_ = 0.0f;
        float optoLevel_ = 0.0f;
        bool envelopeReleasing_ = true;
        bool optoReleasing_ = true;
        float gainReductionDb_ = 0.0f;
        float sidechainEnvelope_ = 0.0f;
        dsp::SidechainFollower sidechainFollower_{};
    };

} // namespace pw8::dynamics
