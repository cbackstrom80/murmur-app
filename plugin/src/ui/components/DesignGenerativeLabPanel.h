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
    /// Figma `murmur-mi-ui-design-generative-lab` (`89:3479`) — Marbles-style generative lab (Track E).
    class DesignGenerativeLabPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit DesignGenerativeLabPanel(MurmurProcessor& processor);
        ~DesignGenerativeLabPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenModMatrix;

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
        void paintClockHero(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintRoutingDiagram(juce::Graphics& g, juce::Rectangle<int> bounds) const;

        MurmurProcessor& processor_;
        bool embeddedInDesignMode_ = false;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;

        SectionPanel clockHeroPanel_{"T / X Clocks"};
        SectionPanel routingPanel_{"6-Out Routing"};
        juce::Rectangle<int> clockHeroBounds_;
        juce::Rectangle<int> routingBounds_;

        GlowRingButton dejaVuButton_{"Deja Vu"};
        GlowRingButton seedLockButton_{"Seed Lock"};
        std::unique_ptr<GlowKnob> tRateKnob_;
        std::unique_ptr<GlowKnob> xRateKnob_;
        std::unique_ptr<GlowKnob> correlationKnob_;
        std::array<std::unique_ptr<GlowKnob>, 3> stream0Knobs_{};
        std::array<std::unique_ptr<GlowKnob>, 3> stream1Knobs_{};

        juce::TextButton openModMatrixButton_{"OPEN MOD MATRIX"};
        juce::Label footerHint_;

        float vizT_ = 0.0f;
        float vizX_ = 0.0f;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dejaVuAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seedLockAttachment_;
    };

} // namespace pw8::plugin::ui
