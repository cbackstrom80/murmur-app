#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ConcentricGlowKnob.h"
#include "GlowRingButton.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-mi-ui-design-utility-peaks` (`89:4076`) — Peaks mini env + mini LFO cards (Track F).
    class DesignUtilityPeaksPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit DesignUtilityPeaksPanel(PatchworkEightProcessor& processor);
        ~DesignUtilityPeaksPanel() override;

        std::function<void()> onClosed;

        void showOverlay();
        void dismiss();
        void setEmbeddedInDesignMode(bool embedded);
        void refreshFromPatch();

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        struct SlotUi
        {
            std::unique_ptr<SectionPanel> panel;
            std::unique_ptr<GlowRingButton> enableButton;
            juce::TextButton envModeButton_{"ENV"};
            juce::TextButton lfoModeButton_{"LFO"};
            std::unique_ptr<ConcentricGlowKnob> envKnob_;
            std::unique_ptr<ConcentricGlowKnob> lfoKnob_;
            juce::TextButton triggerButton_{"TRIG"};
            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment_;
        };

        void timerCallback() override;
        void paintSlotViz(juce::Graphics& g, juce::Rectangle<int> bounds, std::size_t slotIndex) const;
        void setSlotMode(std::size_t slotIndex, int modeIndex);
        void refreshModeButtons(std::size_t slotIndex);

        PatchworkEightProcessor& processor_;
        bool embeddedInDesignMode_ = false;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::Label footerHint_;

        std::array<SlotUi, 2> slots_{};
        std::array<float, 2> vizLevels_{};
        std::array<juce::Rectangle<int>, 2> vizBounds_{};
    };

} // namespace pw8::plugin::ui
