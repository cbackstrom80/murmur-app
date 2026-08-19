#pragma once

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "FxEffectPlayParams.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "LabLauncherChip.h"
#include "MetadataFacetRow.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/FxChainFlowView.h"
#include "wireframe/FxWireframeView.h"

// PLAY-mode FX: 7-slot serial chain (3 layer inserts + 4 master). All 10 DSP
// algorithms are real — see docs/FX_BANK.md. Signal order is fixed by slot
// position; use Swap to reorder within insert or master groups.
namespace pw8::plugin::ui
{
    class DesignFxUiState;

    class FxChainStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit FxChainStrip(PatchworkEightProcessor& processor);
        ~FxChainStrip() override;

        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;

        /// PLAY dashboard: 7-slot chain without VOCODER in TYPE row; vocoder slots open lab.
        void setPlayDashboardMode(bool dashboardMode);
        /// Figma design FX page (35:4 / murmur-fx-*): full-width hero detail editor.
        void setDesignFxPageMode(bool designMode);
        void setDesignFxChipIndex(std::size_t chipIndex);
        void setDesignFxUiState(DesignFxUiState* uiState);
        void selectEngineSlot(std::size_t index);
        void applyDesignModePill(const juce::String& pill);
        std::function<void(std::size_t fxSlotIndex)> onVocoderLabRequested;
        std::function<void(std::size_t fxSlotIndex)> onQuasarLabRequested;
        std::function<void(const juce::String& modePill)> onDesignModeChanged;
        std::function<void()> onDesignUiChanged;

    private:
        struct SlotUi
        {
            juce::String paramPrefix;
            juce::String shortLabel;
            int defaultEnabledType = 1;
            std::unique_ptr<GlowRingButton> selector;
            juce::Label selectorLabel;
            int lastEnabledType = 1;
        };

        void selectSlot(std::size_t index);
        void toggleSelectedSlotEnabled();
        void setSelectedEffectType(int typeOrdinal);
        void rebuildParamKnobs();
        void rebuildDesignFxParamKnobs();
        void syncDesignModeRowFromChip();
        void swapSelectedSlot(int direction);
        [[nodiscard]] SlotUi& selectedSlot();
        [[nodiscard]] const SlotUi& selectedSlot() const;
        void refreshSelectorStates();
        void refreshTransformerUi();
        void refreshCompressorUi();
        void refreshDelaySyncUi();
        void syncTypeRowFromParams();
        void syncTransformerRowsFromParams();
        void syncCompressorRowsFromParams();
        void syncDelaySyncRowsFromParams();
        void setTransformerCore(int coreOrdinal);
        void setTransformerBrand(int brandOrdinal);
        void setCompCharacter(int characterOrdinal);
        void setCompAutoMakeup(bool enabled);
        void updateGainReductionLabel();
        void setDelaySyncEnabled(bool enabled);
        void setDelaySyncDivision(int divisionIndex);
        [[nodiscard]] bool showsMasterCompressorControls() const;
        [[nodiscard]] bool showsCompressorControls() const;
        [[nodiscard]] bool showsDelaySyncControls() const;
        [[nodiscard]] bool canSwapSelectedSlot(int direction) const;
        void updateFlowPrefixes();
        void updatePlayDashboardUi();
        void resizedDashboard(juce::Rectangle<int> content);
        void resizedDesignFxPage(juce::Rectangle<int> content);
        void timerCallback() override;

        bool playDashboardMode_ = false;
        bool designFxPageMode_ = false;
        std::size_t designFxChipIndex_ = 1;
        DesignFxUiState* designFxUiState_ = nullptr;
        std::unique_ptr<LabLauncherChip> vocoderLabChip_;
        std::unique_ptr<LabLauncherChip> quasarLabChip_;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        SectionPanel panel_{"FX Chain — All 10 Algorithms Live"};
        juce::Label helpLabel_;
        wireframe::FxChainFlowView chainFlow_;
        wireframe::FxWireframeView wireframe_;
        std::array<SlotUi, 7> slots_;
        std::size_t selectedSlotIndex_ = 0;

        std::unique_ptr<MetadataFacetRow> typeRow_;
        std::unique_ptr<MetadataFacetRow> designModeRow_;
        juce::Label slotTitleLabel_;
        std::unique_ptr<GlowRingButton> enableButton_;
        juce::TextButton swapLeft_{"◀ Swap"};
        juce::TextButton swapRight_{"Swap ▶"};

        std::unique_ptr<GlowKnob> mixKnob_;
        std::vector<std::unique_ptr<GlowKnob>> paramKnobs_;
        std::vector<GlowKnob*> designKnobGrid_;
        juce::Rectangle<int> designKnobGridBounds_;

        std::unique_ptr<MetadataFacetRow> transCoreRow_;
        std::unique_ptr<MetadataFacetRow> transBrandRow_;
        std::unique_ptr<GlowKnob> transAmountKnob_;

        std::unique_ptr<MetadataFacetRow> compCharacterRow_;
        std::unique_ptr<MetadataFacetRow> compAutoMakeupRow_;
        juce::Label grMeterLabel_;

        std::unique_ptr<MetadataFacetRow> delaySyncRow_;
        std::unique_ptr<MetadataFacetRow> delayDivisionRow_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxChainStrip)
    };

} // namespace pw8::plugin::ui
