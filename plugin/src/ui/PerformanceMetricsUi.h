#pragma once

#include <juce_core/juce_core.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/core/Types.hpp"

namespace pw8::plugin::ui
{
    struct PerformanceMetricsSnapshot
    {
        float cpuPercent = 0.0f;
        int activeVoices = 0;
        int maxVoices = static_cast<int>(core::kMaxVoices);
    };

    [[nodiscard]] inline PerformanceMetricsSnapshot readPerformanceMetrics(
        const PatchworkEightProcessor& processor) noexcept
    {
        PerformanceMetricsSnapshot snapshot;
        snapshot.cpuPercent = processor.getCpuLoadPercent();
        snapshot.activeVoices = processor.getActiveVoiceCount();
        snapshot.maxVoices = static_cast<int>(core::kMaxVoices);
        return snapshot;
    }

    [[nodiscard]] inline juce::String formatCpuPercent(float percent) noexcept
    {
        return juce::String(juce::roundToInt(juce::jlimit(0.0f, 100.0f, percent))) + "%";
    }

    [[nodiscard]] inline juce::String formatVoiceCount(int active, int max) noexcept
    {
        return juce::String(juce::jmax(0, active)) + " / " + juce::String(juce::jmax(1, max));
    }

    [[nodiscard]] inline float cpuBarFillRatio(float percent) noexcept
    {
        return juce::jlimit(0.0f, 1.0f, percent * 0.01f);
    }

    [[nodiscard]] inline float effectTypeLoadWeight(int effectType) noexcept
    {
        switch (effectType)
        {
            case 1: return 0.12f;
            case 2: return 0.18f;
            case 3: return 0.22f;
            case 4: return 0.24f;
            case 5: return 0.26f;
            case 6: return 0.28f;
            case 7: return 0.34f;
            case 8: return 0.14f;
            case 9: return 0.20f;
            case 10: return 0.16f;
            case 11: return 0.30f;
            default: return 0.0f;
        }
    }

    [[nodiscard]] inline float estimateFxLoadPercent(const juce::AudioProcessorValueTreeState& apvts) noexcept
    {
        const auto readF = [&](const char* id, float fallback) {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        };

        const float globalWet = readF(kFxGlobalWetMixId, 1.0f);
        if (readF(kFxGlobalBypassId, 0.0f) >= 0.5f)
            return 0.0f;

        const auto readSlotLoad = [&](const juce::String& prefix) {
            if (prefix.isEmpty())
                return 0.0f;
            const int type =
                apvts.getRawParameterValue(prefix + "Type") != nullptr
                    ? static_cast<int>(apvts.getRawParameterValue(prefix + "Type")->load() + 0.5f)
                    : 0;
            if (type == 0)
                return 0.0f;
            const float mix = readF((prefix + "Mix").toRawUTF8(), 0.0f);
            return effectTypeLoadWeight(type) * juce::jlimit(0.0f, 1.0f, mix * globalWet);
        };

        float load = 0.0f;
        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            load += readSlotLoad(insertFxParamId(slot, ""));
        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            load += readSlotLoad(masterFxParamId(slot, ""));

        const float sendA = readF(kFxSendAId, 0.0f);
        const float sendB = readF(kFxSendBId, 0.0f);
        load += (sendA + sendB) * 0.08f;

        return juce::jlimit(0.0f, 100.0f, load * 100.0f);
    }

    [[nodiscard]] inline float estimateFxLoadPercent(const PatchworkEightProcessor& processor) noexcept
    {
        return estimateFxLoadPercent(processor.apvts);
    }

    [[nodiscard]] inline juce::String formatFxLoadPercent(float percent) noexcept
    {
        return juce::String(juce::roundToInt(juce::jlimit(0.0f, 100.0f, percent))) + "%";
    }

} // namespace pw8::plugin::ui
