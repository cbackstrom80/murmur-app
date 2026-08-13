#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Horizontal arp step strip with live playhead and click-to-toggle editing.
    class ArpStepStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit ArpStepStrip(PatchworkEightProcessor& processor);
        ~ArpStepStrip() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        void timerCallback() override;
        [[nodiscard]] int cellWidthForCount(int count) const noexcept;

        PatchworkEightProcessor& processor_;
        std::size_t lastNumSteps_ = 0;
        std::size_t playheadStep_ = 0;
        std::size_t noteSequenceIndex_ = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpStepStrip)
    };

} // namespace pw8::plugin::ui
