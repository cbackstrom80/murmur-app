#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"

// Peaks-inspired mini utility processors (Track F — envelope + LFO only, no drum mode).

namespace pw8::modulation
{
    enum class PeaksUtilityMode : std::uint8_t
    {
        MiniEnvelope = 0,
        MiniLfo,
    };

    struct PeaksUtilitySlotParams
    {
        bool enabled = false;
        PeaksUtilityMode mode = PeaksUtilityMode::MiniEnvelope;
        float attackMs = 5.0f;
        float releaseMs = 80.0f;
        float lfoRateHz = 0.5f;
        float lfoDepth = 0.5f; ///< 0..1 bipolar depth
    };

    struct PeaksUtilityParams
    {
        std::array<PeaksUtilitySlotParams, 2> slots{};
    };

    struct PeaksUtilityOutputValues
    {
        std::array<float, 2> values{}; ///< 0..1 envelope or bipolar LFO
    };

    class PeaksUtilityProcessor
    {
    public:
        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0; }

        void trigger(std::size_t slotIndex) noexcept
        {
            if (slotIndex >= 2)
                return;
            envLevel_[slotIndex] = 1.0f;
            envReleasing_[slotIndex] = false;
        }

        [[nodiscard]] PeaksUtilityOutputValues processSample(const PeaksUtilityParams& params,
                                                             bool gateHigh) noexcept
        {
            PeaksUtilityOutputValues out;
            const float dt = 1.0f / static_cast<float>(sampleRate_);

            for (std::size_t i = 0; i < 2; ++i)
            {
                const auto& slot = params.slots[i];
                if (!slot.enabled)
                {
                    out.values[i] = 0.0f;
                    continue;
                }

                if (slot.mode == PeaksUtilityMode::MiniEnvelope)
                {
                    if (gateHigh && !envReleasing_[i])
                        envLevel_[i] += dt / (dsp::clamp(slot.attackMs, 0.1f, 5000.0f) * 0.001f);
                    else
                    {
                        envReleasing_[i] = true;
                        envLevel_[i] -= dt / (dsp::clamp(slot.releaseMs, 1.0f, 5000.0f) * 0.001f);
                    }
                    envLevel_[i] = dsp::clamp(envLevel_[i], 0.0f, 1.0f);
                    out.values[i] = envLevel_[i];
                }
                else
                {
                    lfoPhase_[i] += dsp::clamp(slot.lfoRateHz, 0.01f, 40.0f) * dt;
                    if (lfoPhase_[i] >= 1.0f)
                        lfoPhase_[i] -= 1.0f;
                    const float sine = std::sin(lfoPhase_[i] * dsp::kTwoPi);
                    out.values[i] = sine * dsp::clamp(slot.lfoDepth, 0.0f, 1.0f);
                }
            }
            return out;
        }

    private:
        double sampleRate_ = 48000.0;
        std::array<float, 2> envLevel_{};
        std::array<bool, 2> envReleasing_{};
        std::array<float, 2> lfoPhase_{};
    };

} // namespace pw8::modulation
