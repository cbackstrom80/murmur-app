#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "EngineCard.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    class EngineGridPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit EngineGridPanel(PatchworkEightProcessor& processor);
        ~EngineGridPanel() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void setDesignModeV2Layout(bool designMode);
        std::function<void(int engineIndex)> onEngineDoubleClicked;
        std::function<void(int engineIndex)> onWavetableLabRequested;

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        juce::Label headerLabel_;
        juce::Label polyBadgeLabel_;
        juce::Rectangle<int> polyBadgeBounds_;
        std::array<std::unique_ptr<EngineCard>, 8> cards_{};
        bool designModeV2Layout_ = false;
    };

} // namespace pw8::plugin::ui
