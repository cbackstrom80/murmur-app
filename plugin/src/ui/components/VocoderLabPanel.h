#pragma once

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "../PlayModeLayout.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Full-screen vocoder lab — Figma `murmur-vocoder-lab` (15:4).
    class VocoderLabPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit VocoderLabPanel(PatchworkEightProcessor& processor);
        ~VocoderLabPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenFxChain;

        void showForFxSlot(std::size_t slotIndex);
        void dismiss();

        void setEmbeddedInDesignMode(bool embedded);

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void timerCallback() override;
        void rebuildKnobs();
        [[nodiscard]] juce::String slotParamPrefix(std::size_t slotIndex) const;
        [[nodiscard]] juce::String slotDisplayLabel() const;
        void bindSlot(std::size_t slotIndex);
        void paintSignalDiagram(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintBandAnalyzer(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t slotIndex_ = 2;

        juce::TextButton backButton_{"← PLAY BOARD"};
        juce::Label specBadge_;
        juce::Label titleLabel_;
        juce::ComboBox slotCombo_;
        juce::Label sidechainLabel_;
        GlowRingButton enableButton_{"BYPASS OFF"};
        juce::TextButton openFxChainButton_{"OPEN FULL FX CHAIN"};

        std::unique_ptr<GlowKnob> mixKnob_;
        std::vector<std::unique_ptr<GlowKnob>> paramKnobs_;

        juce::Rectangle<int> signalDiagramBounds_;
        juce::Rectangle<int> bandVizBounds_;
        juce::Rectangle<int> controlsCardBounds_;
        juce::Rectangle<int> chainRoutingBounds_;
        juce::Rectangle<int> footerBounds_;

        std::array<float, layout::kDesignVocoderBandCount> carrierBandHeights_{};
        std::array<float, layout::kDesignVocoderBandCount> modulatorBandHeights_{};
        float animPhase_ = 0.0f;
        bool embeddedInDesignMode_ = false;
    };

} // namespace pw8::plugin::ui
