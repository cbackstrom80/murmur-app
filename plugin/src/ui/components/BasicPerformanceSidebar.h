#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "processor/PatchworkEightProcessor.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-basic-view` performance-sidebar (`86:107`) — portamento, 4 macros, stereo VU.
    class BasicPerformanceSidebar : public juce::Component, private juce::Timer
    {
    public:
        explicit BasicPerformanceSidebar(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void refreshFromPatch();

    private:
        void timerCallback() override;
        void rebuildMacroKnobs();
        void paintVerticalMeter(juce::Graphics& g, juce::Rectangle<float> bounds, const scope::VuBallistics& vu,
                                const char* label) const;

        PatchworkEightProcessor& processor_;
        std::unique_ptr<GlowKnob> portamentoKnob_;
        std::vector<std::unique_ptr<GlowKnob>> macroKnobs_;
        scope::VuBallistics leftVu_;
        scope::VuBallistics rightVu_;
        std::array<float, 512> scopeScratch_{};
        juce::Rectangle<int> leftMeterBounds_;
        juce::Rectangle<int> rightMeterBounds_;
    };

} // namespace pw8::plugin::ui
