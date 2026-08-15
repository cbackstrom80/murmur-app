#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../theme/ObsidianPalette.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "MetadataFacetRow.h"
#include "processor/PatchworkEightProcessor.h"
#include "SectionPanel.h"
#include "wireframe/FxChainFlowView.h"

namespace pw8::plugin::ui
{
// PLAY Advanced GLOBAL tab — CHAIN (master FX) and OUTPUT.
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
        Output,
    };

    void setSubTab(SubTab tab);
    void selectMasterSlot(std::size_t localIndex);
    void refreshFromParams();
    [[nodiscard]] int readEffectType(const juce::String& prefix) const;
    void timerCallback() override;

    PatchworkEightProcessor& processor_;
    juce::AudioProcessorValueTreeState& apvts_;
    SectionPanel panel_{"GLOBAL", palette::kAccent};

    SubTab subTab_ = SubTab::Chain;

    juce::TextButton chainTab_{"CHAIN"};
    juce::TextButton outputTab_{"OUTPUT"};

    juce::Label chainHelp_;
    wireframe::FxChainFlowView chainFlow_;
    std::array<std::unique_ptr<GlowRingButton>, 4> masterSelectors_{};
    std::array<juce::Label, 4> masterLabels_{};
    std::unique_ptr<MetadataFacetRow> chainTypeRow_;
    juce::Label chainSlotTitle_;
    std::unique_ptr<GlowRingButton> chainEnable_;
    std::unique_ptr<GlowKnob> chainMixKnob_;
    std::vector<std::unique_ptr<GlowKnob>> chainParamKnobs_;
    std::size_t selectedMasterSlot_ = 0;

    juce::Label outputHelp_;
    std::unique_ptr<GlowKnob> masterGainKnob_;
    std::unique_ptr<GlowKnob> limiterCeilKnob_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalPanel)
};

} // namespace pw8::plugin::ui
