#pragma once

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../visualizer/MurmurVisualizerComponent.h"
#include "../visualizer/PreviewSurface.h"
#include "../visualizer/VisualizerGpu.h"
#include "GlowKnob.h"
#include "processor/MurmurProcessor.h"
#include "pw8/envelope/SegmentEnvelope.hpp"
#include "pw8/modulation/MorphEasing.hpp"
#include "wireframe/EnvelopePathBuilder.h"

namespace pw8::plugin::ui
{
    /// Figma `master-envelope-panel` (`82:4`) on `murmur-design-engine` (`37:787`).
    class MasterEnvelopePanel : public juce::Component, private juce::Timer
    {
    public:
        explicit MasterEnvelopePanel(MurmurProcessor& processor);

        /// Figma `master-envelope-section` (`82:83`) — compact embed above morph timeline.
        void setCompactSectionMode(bool compact);

        /// Figma `murmur-design-engine` (`37:787`) — 180px panel, horizontal ADSR row.
        void setDesignEngineLayout(bool enabled);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        void timerCallback() override;
        void refreshReadouts();
        void refreshSegmentStrip();
        void cycleSelectedSegmentShape();
        void paintEnvelopeCurve(juce::Graphics& g, juce::Rectangle<float> plot) const;

        MurmurProcessor& processor_;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> visualizer_;
        std::unique_ptr<GlowKnob> attackKnob_;
        std::unique_ptr<GlowKnob> decayKnob_;
        std::unique_ptr<GlowKnob> sustainKnob_;
        std::unique_ptr<GlowKnob> releaseKnob_;
        juce::Label attackValueLabel_;
        juce::Label decayValueLabel_;
        juce::Label sustainValueLabel_;
        juce::Label releaseValueLabel_;
        juce::Rectangle<int> curvePlotBounds_;
        juce::Rectangle<int> segmentStripBounds_;
        std::array<juce::Rectangle<int>, pw8::envelope::kMaxSegmentEnvelopeSegments> segmentDotBounds_{};
        std::array<juce::Rectangle<int>, pw8::envelope::kMaxSegmentEnvelopeSegments> segmentEasingChipBounds_{};
        pw8::envelope::SegmentEnvelopeChain segmentChain_{};
        int selectedSegment_ = 0;
        bool compactSectionMode_ = false;
        bool designEngineLayout_ = false;
        mutable preview::PreviewSurface envelopeSurface_;
    };

} // namespace pw8::plugin::ui
