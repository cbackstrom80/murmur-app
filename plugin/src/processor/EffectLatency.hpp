#pragma once

#include <algorithm>
#include <cmath>

#include "pw8/effects/BinauralSpace.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/render/RenderTypes.hpp"

namespace pw8::plugin
{
    [[nodiscard]] inline int latencySamplesForEffectSlot(const effects::EffectSlotParams& p, double sampleRate) noexcept
    {
        if (sampleRate <= 0.0)
            return 0;

        const float sr = static_cast<float>(sampleRate);
        switch (p.type)
        {
            case effects::EffectType::TapeDelay:
                return static_cast<int>(std::ceil(p.tapeDelayMs * 0.001f * sr));
            case effects::EffectType::NodeDelay:
            {
                float maxMs = 0.0f;
                for (const auto& node : p.nodes)
                    maxMs = std::max(maxMs, node.delayMs);
                return static_cast<int>(std::ceil(maxMs * 0.001f * sr));
            }
            case effects::EffectType::FreqShiftEcho:
                return static_cast<int>(std::ceil(p.freqShiftDelayMs * 0.001f * sr));
            case effects::EffectType::FractalEcho:
                return static_cast<int>(std::ceil(p.fractalBaseDelayMs * 0.001f * sr));
            case effects::EffectType::Reverb:
                return static_cast<int>(std::ceil(p.reverbPreDelayMs * 0.001f * sr));
            case effects::EffectType::Limiter:
                return static_cast<int>(std::ceil(p.limiterLookaheadMs * 0.001f * sr));
            case effects::EffectType::BinauralSpace:
                return effects::BinauralSpaceProcessor::latencySamples(sampleRate, p.quasarDelayTimeMs);
            default:
                return 0;
        }
    }

    [[nodiscard]] inline int computeTotalEffectLatencySamples(const patch::Patch& patch, double sampleRate,
                                                               render::QualityMode quality) noexcept
    {
        int total = 0;
        for (const auto& slot : patch.layerA.insertEffects)
            total += latencySamplesForEffectSlot(slot, sampleRate);
        for (const auto& slot : patch.masterEffects)
            total += latencySamplesForEffectSlot(slot, sampleRate);

        if (quality >= render::QualityMode::High)
            total += render::nonlinearOversamplingLatencySamples(quality);

        return total;
    }

} // namespace pw8::plugin
