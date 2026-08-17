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

        /// Width needed to show every active step at the design lane width (scroll when larger than viewport).
        [[nodiscard]] int getPreferredContentWidth() const noexcept;

        /// Recompute lane metrics after the viewport width or step count changes.
        void syncLayoutMetrics(int viewportWidth) noexcept;

    private:
        void timerCallback() override;
        [[nodiscard]] int laneWidthForIndex(int index, int count) const noexcept;
        [[nodiscard]] int laneOffsetForIndex(int index, int count) const noexcept;

        [[nodiscard]] juce::Rectangle<int> laneBounds(int index, int count) const;
        [[nodiscard]] juce::Rectangle<int> velocityLaneBounds(juce::Rectangle<int> lane) const;

        PatchworkEightProcessor& processor_;
        int laneWidth_ = 0;
        int contentWidth_ = 0;
        std::size_t lastNumSteps_ = 0;
        std::size_t playheadStep_ = 0;
        std::size_t noteSequenceIndex_ = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpStepStrip)
    };

} // namespace pw8::plugin::ui
