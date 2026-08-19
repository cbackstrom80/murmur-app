#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "SectionPanel.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-mi-ui-design-dynamics-lab` (`89:2059`) — Streams design lab (Track C).
    class DesignDynamicsLabPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit DesignDynamicsLabPanel(MurmurProcessor& processor);
        ~DesignDynamicsLabPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenPlayOutput;

        void showOverlay();
        void dismiss();
        void setEmbeddedInDesignMode(bool embedded);
        void refreshFromPatch();

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void timerCallback() override;
        void paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintTransferCurve(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintSignalDiagram(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void styleModePill(juce::TextButton& btn, bool active);
        void refreshModePills();
        void setDynamicsMode(int modeIndex);

        MurmurProcessor& processor_;
        bool embeddedInDesignMode_ = false;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;

        SectionPanel heroPanel_{"Transfer Curve"};
        SectionPanel signalPanel_{"Signal Path"};
        juce::Rectangle<int> transferBounds_;
        juce::Rectangle<int> signalBounds_;
        juce::Rectangle<int> transferPlotBounds_;
        juce::Rectangle<int> signalPlotBounds_;

        GlowRingButton enableButton_{"Enable"};
        std::array<juce::TextButton, 4> modePills_{};
        std::unique_ptr<GlowKnob> thresholdKnob_;
        std::unique_ptr<GlowKnob> ratioKnob_;
        std::unique_ptr<GlowKnob> attackKnob_;
        std::unique_ptr<GlowKnob> releaseKnob_;
        std::unique_ptr<GlowKnob> sidechainKnob_;
        std::unique_ptr<GlowKnob> mixKnob_;

        juce::TextButton openPlayOutputButton_{"OPEN IN PLAY OUTPUT"};
        juce::Label footerHint_;

        float grDb_ = 0.0f;
        float sidechainEnv_ = 0.0f;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment_;
    };

} // namespace pw8::plugin::ui
