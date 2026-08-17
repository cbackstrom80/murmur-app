#pragma once

#include <array>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../PlayModeLayout.h"
#include "state/PluginState.h"

// Figma `fx-signal-chain-container` (35:50) — 12-slot BYP→VOC pipeline with IN/OUT terminals.
namespace pw8::plugin::ui
{
    class DesignFxUiState;

    class DesignFxSignalChain : public juce::Component, private juce::Timer
    {
    public:
        explicit DesignFxSignalChain(juce::AudioProcessorValueTreeState& apvts);

        void setSelectedChip(std::size_t chipIndex);
        void setUiState(DesignFxUiState* uiState) { uiState_ = uiState; }
        [[nodiscard]] std::size_t getSelectedChip() const noexcept { return selectedChip_; }

        std::function<void(std::size_t chipIndex)> onChipSelected;
        std::function<void()> onDisplayOrderChanged;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void resized() override;

    private:
        struct ChipDef
        {
            const char* label;
            int effectType;
            int engineSlot;
            const char* slotDisplay;
            bool disabled;
        };

        void timerCallback() override;
        [[nodiscard]] std::size_t displayIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] std::size_t chipIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] juce::Rectangle<float> tileBoundsForDisplayIndex(std::size_t displayIndex) const;
        void paintChip(juce::Graphics& g, juce::Rectangle<float> tile, const ChipDef& def, std::size_t index,
                       int liveType, float mix, bool selected, bool dragGhost) const;

        juce::AudioProcessorValueTreeState& apvts_;
        DesignFxUiState* uiState_ = nullptr;
        std::size_t selectedChip_ = 1;
        std::size_t dragFromDisplay_ = static_cast<std::size_t>(-1);
        std::size_t dragHoverDisplay_ = static_cast<std::size_t>(-1);
        bool dragging_ = false;
        std::array<int, layout::kDesignFxPageSlotCount> liveTypes_{};
        std::array<float, layout::kDesignFxPageSlotCount> mixLevels_{};
        float fxLoadPercent_ = 0.0f;

        static constexpr std::array<ChipDef, layout::kDesignFxPageSlotCount> kChips{{
            {"BYP", 0, 0, "I1", false},
            {"SAT", 1, 0, "I1", false},
            {"CHR", 2, 1, "I2", false},
            {"TAPE", 3, 2, "I3", false},
            {"MOOD", 8, 2, "I5", false},
            {"FSHF", 5, 3, "M1", false},
            {"FRAC", 6, 4, "M2", false},
            {"REV", 7, 2, "I3", false},
            {"EQ", 8, 5, "M3", false},
            {"COMP", 9, 6, "M4", false},
            {"LIM", 10, 6, "M4", false},
            {"VOC", 11, 2, "I3", false},
        }};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignFxSignalChain)
    };

    [[nodiscard]] inline juce::String designFxEngineSlotPrefix(int engineSlot)
    {
        if (engineSlot < 0)
            return {};
        if (engineSlot < 3)
            return pw8::plugin::insertFxParamId(static_cast<std::size_t>(engineSlot), "");
        return pw8::plugin::masterFxParamId(static_cast<std::size_t>(engineSlot - 3), "");
    }

} // namespace pw8::plugin::ui
