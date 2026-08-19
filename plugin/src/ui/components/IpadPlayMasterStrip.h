#pragma once

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "ObsidianEnvelopeVisualizer.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Compact master motion row for Figma `ipad-play-view` — envelope + portamento + global LPF.
    class IpadPlayMasterStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit IpadPlayMasterStrip(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        ObsidianEnvelopeVisualizer visualizer_;
        std::unique_ptr<GlowKnob> attackKnob_;
        std::unique_ptr<GlowKnob> decayKnob_;
        std::unique_ptr<GlowKnob> sustainKnob_;
        std::unique_ptr<GlowKnob> releaseKnob_;
        std::unique_ptr<GlowKnob> portamentoKnob_;
        std::unique_ptr<GlowKnob> cutoffKnob_;
        std::unique_ptr<GlowKnob> resonanceKnob_;
        juce::Rectangle<int> curvePlotBounds_;
    };

} // namespace pw8::plugin::ui
