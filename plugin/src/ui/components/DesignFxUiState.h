#pragma once

#include <array>
#include <cstddef>

#include <juce_core/juce_core.h>

namespace pw8::plugin::ui
{
    /// UI-only design-FX knob values (0..1) for labels that have no APVTS field yet.
    class DesignFxUiState
    {
    public:
        static constexpr std::size_t kKnobsPerChip = 6;

        [[nodiscard]] float knobValue(std::size_t chipIndex, std::size_t knobIndex) const
        {
            if (chipIndex >= byChip_.size() || knobIndex >= kKnobsPerChip)
                return 0.5f;
            return byChip_[chipIndex][knobIndex];
        }

        void setKnobValue(std::size_t chipIndex, std::size_t knobIndex, float normalized)
        {
            if (chipIndex >= byChip_.size() || knobIndex >= kKnobsPerChip)
                return;
            byChip_[chipIndex][knobIndex] = juce::jlimit(0.0f, 1.0f, normalized);
        }

        [[nodiscard]] juce::String moodPill() const { return moodPill_; }

        void setMoodPill(const juce::String& pill)
        {
            if (pill.isNotEmpty())
                moodPill_ = pill;
        }

        [[nodiscard]] bool eqAnalyzerActive() const { return eqAnalyzerActive_; }

        void setEqAnalyzerActive(bool active) { eqAnalyzerActive_ = active; }

        [[nodiscard]] bool limTruePeakActive() const { return limTruePeakActive_; }

        void setLimTruePeakActive(bool active) { limTruePeakActive_ = active; }

        void saveUserPreferences() const;
        void loadUserPreferences();

        static constexpr std::size_t kChipCount = 12;

        void resetChipDisplayOrder()
        {
            for (std::size_t i = 0; i < kChipCount; ++i)
                chipDisplayOrder_[i] = i;
        }

        [[nodiscard]] std::size_t chipAtDisplayIndex(std::size_t displayIndex) const
        {
            if (displayIndex >= kChipCount)
                return 0;
            return chipDisplayOrder_[displayIndex];
        }

        [[nodiscard]] std::size_t displayIndexForChip(std::size_t chipIndex) const
        {
            for (std::size_t d = 0; d < kChipCount; ++d)
            {
                if (chipDisplayOrder_[d] == chipIndex)
                    return d;
            }
            return chipIndex;
        }

        void reorderChipDisplay(std::size_t fromDisplay, std::size_t toDisplay)
        {
            if (fromDisplay == 0 || toDisplay == 0 || fromDisplay >= kChipCount || toDisplay >= kChipCount
                || fromDisplay == toDisplay)
                return;

            const std::size_t chipId = chipDisplayOrder_[fromDisplay];
            std::array<std::size_t, kChipCount> next = chipDisplayOrder_;
            if (fromDisplay < toDisplay)
            {
                for (std::size_t i = fromDisplay; i < toDisplay; ++i)
                    next[i] = chipDisplayOrder_[i + 1];
                next[toDisplay] = chipId;
            }
            else
            {
                for (std::size_t i = fromDisplay; i > toDisplay; --i)
                    next[i] = chipDisplayOrder_[i - 1];
                next[toDisplay] = chipId;
            }
            chipDisplayOrder_ = next;
        }

        DesignFxUiState() { resetChipDisplayOrder(); }

    private:
        std::array<std::array<float, kKnobsPerChip>, 12> byChip_{};
        std::array<std::size_t, kChipCount> chipDisplayOrder_{};
        juce::String moodPill_{"WARM"};
        bool eqAnalyzerActive_ = true;
        bool limTruePeakActive_ = true;
    };

} // namespace pw8::plugin::ui
