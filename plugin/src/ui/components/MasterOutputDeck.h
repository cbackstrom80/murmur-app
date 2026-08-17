#pragma once

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "processor/PatchworkEightProcessor.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Figma `ipad-play-view` master-output-deck — large master knob + stereo meters.
    class MasterOutputDeck : public juce::Component, private juce::Timer
    {
    public:
        explicit MasterOutputDeck(PatchworkEightProcessor& processor);
        ~MasterOutputDeck() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void paintVerticalMeter(juce::Graphics& g, juce::Rectangle<float> bounds, const scope::VuBallistics& vu,
                                const char* label) const;

        PatchworkEightProcessor& processor_;
        std::unique_ptr<GlowKnob> masterKnob_;
        scope::VuBallistics leftVu_;
        scope::VuBallistics rightVu_;
        std::array<float, 512> scopeScratch_{};
        juce::Rectangle<int> leftMeterBounds_;
        juce::Rectangle<int> rightMeterBounds_;
    };

} // namespace pw8::plugin::ui
