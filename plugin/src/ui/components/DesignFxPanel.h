#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "DesignFxCardBrowser.h"
#include "DesignFxPresetLibrary.h"
#include "DesignFxSignalChain.h"
#include "DesignFxUiState.h"
#include "FxChainStrip.h"
#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/DesignFxHeroViz.h"

namespace pw8::plugin::ui
{
    /// Design FX — card browser (152:4) default, chain editor (35:4), and murmur-fx-* detail.
    class DesignFxPanel : public juce::Component, private juce::Timer
    {
    public:
        enum class FxViewMode
        {
            Cards,
            Chain,
            Detail,
        };

        DesignFxPanel(PatchworkEightProcessor& processor, ModAssignmentController& modAssignmentController);

        std::function<void()> onClosed;
        std::function<void(std::size_t fxSlotIndex)> onVocoderLabRequested;
        std::function<void(std::size_t fxSlotIndex)> onQuasarLabRequested;

        void setEmbeddedInDesignMode(bool embedded);
        void bindSelectedChip(std::size_t chipIndex);
        void setFxViewMode(FxViewMode mode);
        [[nodiscard]] FxViewMode getFxViewMode() const noexcept { return viewMode_; }

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void applyViewVisibility();
        void openCard(int cardIndex);
        void paintRoutingBar(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintFocusedHeader(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintCardBrowserStatusBar(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintCardBrowserModChips(juce::Graphics& g) const;
        [[nodiscard]] bool modSourceActive(modulation::ModSource source) const;
        void paintViewToggleGroup(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        [[nodiscard]] juce::String selectedChipTitle() const;
        [[nodiscard]] juce::String focusedStatusLine() const;
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
        void updateToggleButtons();

        PatchworkEightProcessor& processor_;
        bool embeddedInDesignMode_ = false;
        FxViewMode viewMode_ = FxViewMode::Cards;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label presetLabel_;

        juce::Label modulesLabel_;
        juce::Label modulesHintLabel_;
        juce::TextButton cardsToggle_{"CARDS"};
        juce::TextButton chainToggle_{"CHAIN"};
        juce::TextButton detailBackButton_{"← FX CARDS"};

        DesignFxCardBrowser cardBrowser_;
        DesignFxSignalChain signalChain_;
        FxChainStrip detailStrip_;
        wireframe::DesignFxHeroViz heroViz_;

        juce::Rectangle<int> subHeaderBounds_;
        juce::Rectangle<int> statusBarBounds_;
        juce::Rectangle<int> viewToggleBounds_;
        std::array<juce::Rectangle<int>, 4> statusModChipBounds_{};
        juce::TextButton statusPanicButton_{"PANIC RESET"};
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
