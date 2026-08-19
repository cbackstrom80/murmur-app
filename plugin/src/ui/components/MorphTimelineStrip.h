#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma `morph-timeline-panel` (`89:736`) — hue ring track, autoplay chips, FR.STEP tick flash.
    class MorphTimelineStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit MorphTimelineStrip(MurmurProcessor& processor);

        std::function<void(std::size_t keyframeIndex)> onKeyframeSelected;

        void setShowMorphKnob(bool show);
        void setCompactHubMode(bool compact);
        void refresh();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        void timerCallback() override;
        [[nodiscard]] juce::Rectangle<float> gradientTrackBounds() const noexcept;
        [[nodiscard]] float positionFromX(int x) const noexcept;
        [[nodiscard]] int xFromPosition(float position) const noexcept;
        [[nodiscard]] int hitKeyframeIndex(juce::Point<int> pos) const;
        void setMorphPosition(float position);
        void paintHeader(juce::Graphics& g, juce::Rectangle<int> area) const;
        void paintTrack(juce::Graphics& g, juce::Rectangle<int> area);
        void paintChip(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text,
                       juce::Colour accent) const;

        MurmurProcessor& processor_;
        std::unique_ptr<GlowKnob> morphKnob_;
        bool showMorphKnob_ = true;
        bool compactHubMode_ = false;
        bool draggingPlayhead_ = false;
        int frStepFlashKeyframe_ = -1;
        juce::Rectangle<int> trackBounds_;
        juce::Rectangle<int> headerBounds_;
    };

} // namespace pw8::plugin::ui
