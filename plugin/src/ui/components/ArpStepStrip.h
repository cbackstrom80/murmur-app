#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Horizontal arp step strip (1–64 steps) with paging, live playhead, and click/drag editing.
    class ArpStepStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit ArpStepStrip(PatchworkEightProcessor& processor);
        ~ArpStepStrip() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;

        [[nodiscard]] int getPreferredContentWidth() const noexcept;
        void syncLayoutMetrics(int viewportWidth) noexcept;

        void setPageStart(int pageStartStep) noexcept;
        [[nodiscard]] int pageStart() const noexcept { return pageStart_; }

    private:
        void timerCallback() override;
        [[nodiscard]] int visibleStepCount(int totalSteps) const noexcept;
        [[nodiscard]] int laneWidthForLayout(int totalSteps) const noexcept;
        [[nodiscard]] int laneWidthForIndex(int index, int count) const noexcept;
        [[nodiscard]] int laneOffsetForIndex(int index, int count) const noexcept;
        [[nodiscard]] int globalStepIndexForLocal(int localIndex) const noexcept;

        [[nodiscard]] juce::Rectangle<int> laneBounds(int localIndex, int count) const;
        [[nodiscard]] juce::Rectangle<int> velocityLaneBounds(juce::Rectangle<int> lane) const;
        [[nodiscard]] int stepIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] int totalSteps() const noexcept;
        [[nodiscard]] int addLaneLocalIndex(int total) const noexcept;
        [[nodiscard]] juce::Rectangle<int> addLaneBounds(int localIndex, int count) const;
        void paintAddStepLane(juce::Graphics& g, juce::Rectangle<int> lane) const;

        void paintStepLane(juce::Graphics& g, int localIndex, int globalIndex, juce::Rectangle<int> lane,
                           bool isPlayhead) const;

        PatchworkEightProcessor& processor_;
        int laneWidth_ = 0;
        int contentWidth_ = 0;
        int pageStart_ = 0;
        std::size_t lastNumSteps_ = 0;
        std::size_t playheadStep_ = 0;
        std::size_t noteSequenceIndex_ = 0;
        float pulsePhase_ = 0.0f;
        int dragStepIndex_ = -1;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpStepStrip)
    };

} // namespace pw8::plugin::ui
