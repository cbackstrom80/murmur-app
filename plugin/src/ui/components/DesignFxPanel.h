#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "DesignFxPresetLibrary.h"
#include "DesignFxSignalChain.h"
#include "DesignFxUiState.h"
#include "FxChainStrip.h"
#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/DesignFxHeroViz.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-design-fx` (35:4) — standalone 12-slot FX rack for design sub-nav.
    class DesignFxPanel : public juce::Component, private juce::Timer
    {
    public:
        DesignFxPanel(PatchworkEightProcessor& processor, ModAssignmentController& modAssignmentController);

        std::function<void()> onClosed;
        std::function<void(std::size_t fxSlotIndex)> onVocoderLabRequested;

        void setEmbeddedInDesignMode(bool embedded);
        void bindSelectedChip(std::size_t chipIndex);

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void paintRoutingBar(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintFocusedHeader(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        [[nodiscard]] juce::String selectedChipTitle() const;
        [[nodiscard]] int selectedEngineSlot() const;
        [[nodiscard]] juce::String selectedParamPrefix() const;
        void applyDesignPreset(std::size_t presetIndex);
        void saveCurrentPreset();
        void renameCurrentPreset();
        void deleteCurrentPreset();
        void showPresetMenu(juce::Point<int> anchor);
        void syncPresetLabelFromChip();
        [[nodiscard]] juce::String currentModePillForChip() const;
        void syncStubKnobsToApvts();

        PatchworkEightProcessor& processor_;
        bool embeddedInDesignMode_ = false;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label presetLabel_;

        DesignFxSignalChain signalChain_;
        FxChainStrip detailStrip_;
        wireframe::DesignFxHeroViz heroViz_;

        juce::Rectangle<int> routingBounds_;
        juce::Rectangle<int> detailChromeBounds_;
        juce::Rectangle<int> headerBounds_;
        juce::Rectangle<int> mutable activeToggleBounds_;
        juce::Rectangle<int> mutable presetChipBounds_;

        juce::Rectangle<int> mutable prePostButtonBounds_;
        juce::Rectangle<int> mutable bypassButtonBounds_;
        juce::Rectangle<int> mutable globalWetTrackBounds_;
        juce::Rectangle<int> mutable sendATrackBounds_;
        juce::Rectangle<int> mutable sendBTrackBounds_;
        juce::Rectangle<int> mutable sidechainChipBounds_;
        enum class DragTarget
        {
            None,
            GlobalWet,
            SendA,
            SendB,
        };
        DragTarget activeDrag_ = DragTarget::None;

        std::size_t selectedChip_ = 1;
        std::array<std::size_t, layout::kDesignFxPageSlotCount> presetIndices_{};
        DesignFxPresetLibrary presetLibrary_;
        DesignFxUiState uiState_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignFxPanel)
    };

} // namespace pw8::plugin::ui
