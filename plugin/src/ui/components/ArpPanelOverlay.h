#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ArpStepStrip.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Right-side slide-in drawer for arpeggiator scalar controls (P0).
    class ArpPanelOverlay : public juce::Component
    {
    public:
        explicit ArpPanelOverlay(PatchworkEightProcessor& processor);

        std::function<void()> onClosed;

        void showDrawer();
        void dismiss();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        juce::Component drawer_;
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::TextButton closeButton_{"CLOSE"};
        GlowRingButton enableButton_{"ON"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment_;
        std::unique_ptr<GlowKnob> modeKnob_;
        std::unique_ptr<GlowKnob> rateModeKnob_;
        std::unique_ptr<GlowKnob> rateHzKnob_;
        std::unique_ptr<GlowKnob> syncDivisionKnob_;
        std::unique_ptr<GlowKnob> octaveRangeKnob_;
        std::unique_ptr<GlowKnob> numStepsKnob_;
        std::unique_ptr<GlowKnob> latchKnob_;
        ArpStepStrip stepStrip_;
    };

} // namespace pw8::plugin::ui
