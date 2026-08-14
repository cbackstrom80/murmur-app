#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// Scaled-down FDN room core shared by the full Reverb slot and Quasar embedded
// paths (see docs/GLOBAL_QUASAR_FX_PLAN.md §2.4). Uses the same 8-line
// Householder-matrix discipline as effects::ReverbProcessor but without the
// M7 pre-delay / diffuser / early-reflection stages — a lighter tail for
// per-path spatial wash.
namespace pw8::effects
{
    struct RoomParams
    {
        float size = 1.0f;      ///< 0.2..3, scales delay-line lengths.
        float damping = 0.55f;  ///< 0..1, HF decay (higher = darker tail).
        float amount = 0.45f;   ///< 0..1, wet contribution to path output.
    };

    class RoomEngine
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            for (auto& line : lines_)
                line.prepare(sampleRate, kMaxRoomLineSeconds);
            reset();
        }

        void reset() noexcept
        {
            for (auto& line : lines_)
                line.reset();
            for (auto& f : absorptionHigh_)
                f.reset();
            modPhase_.fill(0.0f);
        }

        void processMono(float in, const RoomParams& p, float& wetL, float& wetR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float sizeScale = dsp::clamp(p.size, 0.2f, 3.0f);
            const float amount = dsp::clamp(p.amount, 0.0f, 1.0f);
            if (amount <= 1.0e-6f)
            {
                wetL = wetR = 0.0f;
                return;
            }

            // Damping maps to HF RT60 multiplier: 0 = bright, 1 = dark.
            const float hfRatio = dsp::lerp(1.0f, 0.25f, dsp::clamp(p.damping, 0.0f, 1.0f));
            const float rtMid = 1.2f + sizeScale * 0.8f;
            const float rtHigh = std::max(rtMid * hfRatio, 0.05f);

            std::array<float, kNumRoomEngineLines> lineOut{};
            std::array<float, kNumRoomEngineLines> delaySamples{};
            const float modDepthMs = 1.5f;
            for (std::size_t i = 0; i < kNumRoomEngineLines; ++i)
            {
                modPhase_[i] += dsp::kTwoPi * 0.35f * kModRateRatio[i] / sr;
                if (modPhase_[i] > dsp::kTwoPi)
                    modPhase_[i] -= dsp::kTwoPi;
                const float modOffsetMs = modDepthMs * std::sin(modPhase_[i] + kModPhaseOffset[i]);
                const float baseMs = kBaseDelayMs[i] * sizeScale;
                delaySamples[i] = dsp::clamp((baseMs + modOffsetMs) * 0.001f * sr, 1.0f,
                                              sr * kMaxRoomLineSeconds - 4.0f);
                lineOut[i] = lines_[i].readInterpolated(delaySamples[i]);
            }

            float sum = 0.0f;
            for (const float v : lineOut)
                sum += v;
            const float householderFactor = 2.0f / static_cast<float>(kNumRoomEngineLines);

            wetL = 0.0f;
            wetR = 0.0f;
            const float tapGain = 2.0f / static_cast<float>(kNumRoomEngineLines);
            for (std::size_t i = 0; i < kNumRoomEngineLines; ++i)
            {
                const float mixed = lineOut[i] - householderFactor * sum;

                const float lineSeconds = delaySamples[i] / sr;
                const float gMid = std::pow(10.0f, -3.0f * lineSeconds / rtMid);
                const float gHigh = std::pow(10.0f, -3.0f * lineSeconds / rtHigh);
                const float highShelfDb = dsp::gainToDb(gHigh / std::max(gMid, 1.0e-9f));

                absorptionHigh_[i].setHighShelf(4500.0f, highShelfDb, sampleRate_);
                const float absorbed = gMid * absorptionHigh_[i].renderSample(mixed);

                lines_[i].write(in * 0.5f + absorbed);

                if (i % 2 == 0)
                    wetL += lineOut[i] * tapGain;
                else
                    wetR += lineOut[i] * tapGain;
            }

            wetL *= amount;
            wetR *= amount;
        }

    private:
        static constexpr std::array<float, kNumRoomEngineLines> kBaseDelayMs = {
            19.3f, 23.7f, 27.1f, 31.3f, 35.9f, 39.7f, 43.1f, 47.9f};
        static constexpr std::array<float, kNumRoomEngineLines> kModRateRatio = {
            1.00f, 1.11f, 0.93f, 1.05f, 0.97f, 1.17f, 0.89f, 1.03f};
        static constexpr std::array<float, kNumRoomEngineLines> kModPhaseOffset = {
            0.0f, 0.79f, 1.57f, 2.36f, 3.14f, 3.93f, 4.71f, 5.50f};

        double sampleRate_ = 48000.0;
        std::array<dsp::DelayLine, kNumRoomEngineLines> lines_{};
        std::array<dsp::Biquad, kNumRoomEngineLines> absorptionHigh_{};
        std::array<float, kNumRoomEngineLines> modPhase_{};
    };

} // namespace pw8::effects
