#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "FxEffectPlayParams.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
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
    class FxChainStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit FxChainStrip(PatchworkEightProcessor& processor);
        ~FxChainStrip() override;

        void resized() override;

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
        void swapSelectedSlot(int direction);
        [[nodiscard]] SlotUi& selectedSlot();
        [[nodiscard]] const SlotUi& selectedSlot() const;
        void refreshSelectorStates();
        void refreshTransformerUi();
        void syncTypeRowFromParams();
        void syncTransformerRowsFromParams();
        void setTransformerCore(int coreOrdinal);
        void setTransformerBrand(int brandOrdinal);
        [[nodiscard]] bool showsMasterCompressorControls() const;
        [[nodiscard]] bool canSwapSelectedSlot(int direction) const;
        void updateFlowPrefixes();
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        SectionPanel panel_{"FX Chain — All 10 Algorithms Live"};
        juce::Label helpLabel_;
        wireframe::FxChainFlowView chainFlow_;
        wireframe::FxWireframeView wireframe_;
        std::array<SlotUi, 7> slots_;
        std::size_t selectedSlotIndex_ = 0;

        std::unique_ptr<MetadataFacetRow> typeRow_;
        juce::Label slotTitleLabel_;
        std::unique_ptr<GlowRingButton> enableButton_;
        juce::TextButton swapLeft_{"◀ Swap"};
        juce::TextButton swapRight_{"Swap ▶"};

        std::unique_ptr<GlowKnob> mixKnob_;
        std::vector<std::unique_ptr<GlowKnob>> paramKnobs_;

        std::unique_ptr<MetadataFacetRow> transCoreRow_;
        std::unique_ptr<MetadataFacetRow> transBrandRow_;
        std::unique_ptr<GlowKnob> transAmountKnob_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxChainStrip)
    };

} // namespace pw8::plugin::ui
