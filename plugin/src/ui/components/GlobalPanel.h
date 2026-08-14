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

// PLAY Advanced GLOBAL tab — CHAIN (master FX), QUASAR (deep spatial), OUTPUT.
// See docs/GLOBAL_QUASAR_FX_PLAN.md §4.
namespace pw8::plugin::ui
{
    class GlobalPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit GlobalPanel(PatchworkEightProcessor& processor);
        ~GlobalPanel() override;

        void resized() override;

    private:
        enum class SubTab
        {
            Chain,
            Quasar,
            Output,
        };

        void setSubTab(SubTab tab);
        void selectMasterSlot(std::size_t localIndex);
        void refreshFromParams();
        void rebuildQuasarKnobs(const juce::String& prefix);
        [[nodiscard]] int findQuasarMasterSlot() const;
        [[nodiscard]] int readEffectType(const juce::String& prefix) const;
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        SubTab subTab_ = SubTab::Quasar;

        SectionPanel panel_{"GLOBAL — Master Bus"};
        juce::TextButton chainTab_{"CHAIN"};
        juce::TextButton quasarTab_{"QUASAR"};
        juce::TextButton outputTab_{"OUTPUT"};

        // CHAIN sub-tab
        juce::Label chainHelp_;
        wireframe::FxChainFlowView chainFlow_;
        std::array<std::unique_ptr<GlowRingButton>, 4> masterSelectors_;
        std::array<juce::Label, 4> masterLabels_;
        std::size_t selectedMasterSlot_ = 2;
        std::unique_ptr<MetadataFacetRow> chainTypeRow_;
        juce::Label chainSlotTitle_;
        std::unique_ptr<GlowRingButton> chainEnable_;
        std::unique_ptr<GlowKnob> chainMixKnob_;
        std::vector<std::unique_ptr<GlowKnob>> chainParamKnobs_;

        // QUASAR sub-tab
        juce::Label quasarHelp_;
        juce::Label quasarScopeLabel_;
        juce::Label quasarSlotLabel_;
        juce::String quasarPrefix_;
        std::vector<std::unique_ptr<GlowKnob>> quasarKnobs_;

        // OUTPUT sub-tab
        juce::Label outputHelp_;
        std::unique_ptr<GlowKnob> masterGainKnob_;
        std::unique_ptr<GlowKnob> limiterCeilKnob_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalPanel)
    };

} // namespace pw8::plugin::ui
