#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"

// Marbles-inspired generative mod sources (Track E — docs/MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md §9).
// T/X stochastic clocks, deja-vu from patch seed, lag-smoothed bipolar outputs for mod matrix.

namespace pw8::modulation
{
    inline constexpr std::size_t kNumGenerativeStreams = 4;
    inline constexpr std::size_t kNumGenerativeOutputs = 6;

    struct GenerativeStreamParams
    {
        float spread = 0.5f; ///< 0..1 — output range narrowing
        float bias = 0.0f;   ///< -1..1 — DC offset on bipolar output
        float lagMs = 80.0f;
        std::string quantScale; ///< reserved for scale-aware quant (E-M2)
    };

    struct GenerativeParams
    {
        std::uint64_t seed = 0; ///< 0 = inherit Patch::seed
        bool dejaVu = true;
        bool seedLocked = false;
        float clockTRateHz = 0.47f;
        float clockXRateHz = 4.7f;
        float correlation = 0.0f; ///< 0..1 — X raw blends toward T raw
        std::array<GenerativeStreamParams, kNumGenerativeStreams> streams{};
        /// Six Marbles-style outputs: [T, X, stream0..3] default routing indices.
        std::array<std::uint8_t, kNumGenerativeOutputs> outputRouting{0, 1, 2, 3, 4, 5};
    };

    struct GenerativeOutputValues
    {
        float randomT = 0.0f;
        float randomX = 0.0f;
        std::array<float, 4> outputs{}; ///< bipolar -1..1
    };

    class GenerativeProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        void reset(const GenerativeParams& params, std::uint64_t patchSeed) noexcept
        {
            const std::uint64_t effectiveSeed = resolveSeed(params, patchSeed);
            if (dejaVuActive_ && effectiveSeed == activeSeed_ && params.dejaVu)
                return;

            activeSeed_ = effectiveSeed;
            dejaVuActive_ = params.dejaVu;
            rng_.reseed(effectiveSeed ^ 0x4D4152424C4553ULL); // "MARBLE"
            tPhase_ = 0.0f;
            xPhase_ = 0.0f;
            rawT_ = 0.0f;
            rawX_ = 0.0f;
            rawOutputs_.fill(0.0f);
            lagState_.fill(0.0f);
            shapedT_ = 0.0f;
            shapedX_ = 0.0f;
            shapedOutputs_.fill(0.0f);
        }

        [[nodiscard]] GenerativeOutputValues processSample(const GenerativeParams& params,
                                                           std::uint64_t patchSeed) noexcept
        {
            if (!params.seedLocked)
            {
                const std::uint64_t effectiveSeed = resolveSeed(params, patchSeed);
                if (effectiveSeed != activeSeed_)
                    reset(params, patchSeed);
            }

            const float dt = 1.0f / static_cast<float>(sampleRate_);
            tPhase_ += params.clockTRateHz * dt;
            xPhase_ += params.clockXRateHz * dt;

            if (tPhase_ >= 1.0f)
            {
                tPhase_ -= 1.0f;
                rawT_ = rng_.nextFloat();
                for (std::size_t i = 0; i < kNumGenerativeStreams; ++i)
                    rawOutputs_[i] = rng_.nextFloat();
            }

            if (xPhase_ >= 1.0f)
            {
                xPhase_ -= 1.0f;
                rawX_ = rng_.nextFloat();
            }

            const float corr = dsp::clamp(params.correlation, 0.0f, 1.0f);
            const float blendedX = dsp::lerp(rawX_, rawT_, corr);

            shapedT_ = applyStreamShaping(rawT_, params.streams[0], dt, lagStateT_);
            shapedX_ = applyStreamShaping(blendedX, params.streams[1], dt, lagStateX_);

            GenerativeOutputValues out;
            out.randomT = shapedT_;
            out.randomX = shapedX_;
            for (std::size_t i = 0; i < kNumGenerativeStreams; ++i)
            {
                shapedOutputs_[i] =
                    applyStreamShaping(rawOutputs_[i], params.streams[i], dt, lagState_[i]);
                out.outputs[i] = shapedOutputs_[i];
            }
            return out;
        }

        [[nodiscard]] std::array<float, kNumGenerativeOutputs> routedOutputs(
            const GenerativeOutputValues& values, const GenerativeParams& params) const noexcept
        {
            const std::array<float, kNumGenerativeOutputs> bus{values.randomT, values.randomX,
                                                               values.outputs[0], values.outputs[1],
                                                               values.outputs[2], values.outputs[3]};
            std::array<float, kNumGenerativeOutputs> routed{};
            for (std::size_t i = 0; i < kNumGenerativeOutputs; ++i)
            {
                const std::uint8_t idx = params.outputRouting[i] < kNumGenerativeOutputs
                                             ? params.outputRouting[i]
                                             : static_cast<std::uint8_t>(i);
                routed[i] = bus[idx];
            }
            return routed;
        }

    private:
        [[nodiscard]] static std::uint64_t resolveSeed(const GenerativeParams& params,
                                                       std::uint64_t patchSeed) noexcept
        {
            return params.seed != 0 ? params.seed : (patchSeed != 0 ? patchSeed : 0xC0FFEEULL);
        }

        [[nodiscard]] static float applyStreamShaping(float raw01, const GenerativeStreamParams& stream, float dt,
                                                      float& lagState) noexcept
        {
            const float spread = dsp::clamp(stream.spread, 0.0f, 1.0f);
            const float bias = dsp::clamp(stream.bias, -1.0f, 1.0f);
            const float centered = (raw01 - 0.5f) * 2.0f;
            const float spreadValue = centered * spread + bias;
            const float target = dsp::clamp(spreadValue, -1.0f, 1.0f);

            const float lagMs = dsp::clamp(stream.lagMs, 0.1f, 5000.0f);
            const float tau = lagMs * 0.001f;
            const float coeff = std::exp(-dt / tau);
            lagState = target + (lagState - target) * coeff;
            return lagState;
        }

        double sampleRate_ = 48000.0;
        dsp::DeterministicRng rng_;
        std::uint64_t activeSeed_ = 0;
        bool dejaVuActive_ = true;

        float tPhase_ = 0.0f;
        float xPhase_ = 0.0f;
        float rawT_ = 0.0f;
        float rawX_ = 0.0f;
        std::array<float, 4> rawOutputs_{};
        float lagStateT_ = 0.0f;
        float lagStateX_ = 0.0f;
        std::array<float, 4> lagState_{};
        float shapedT_ = 0.0f;
        float shapedX_ = 0.0f;
        std::array<float, 4> shapedOutputs_{};
    };

} // namespace pw8::modulation
