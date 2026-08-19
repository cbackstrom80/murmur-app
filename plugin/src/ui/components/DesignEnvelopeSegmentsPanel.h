#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowRingButton.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "pw8/envelope/SegmentEnvelope.hpp"

namespace pw8::plugin::ui
{
    /// Figma `murmur-mi-ui-design-envelope-segments` (`89:2712`) — Stages design lab (Track D).
    class DesignEnvelopeSegmentsPanel : public juce::Component
    {
    public:
        explicit DesignEnvelopeSegmentsPanel(PatchworkEightProcessor& processor);
        ~DesignEnvelopeSegmentsPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenPlayMotion;

        void showOverlay();
        void dismiss();
        void setEmbeddedInDesignMode(bool embedded);
        void refreshFromPatch();

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void styleEnvPill(juce::TextButton& btn, bool active);
        void styleSegmentChip(juce::TextButton& btn, bool selected);
        void refreshEnvPills();
        void refreshSegmentEditor();
        void setSelectedEnv(std::size_t envIndex);
        void setSelectedSegment(std::size_t segmentIndex);
        void cycleSegmentType(std::size_t segmentIndex);
        void cycleSegmentShape(std::size_t segmentIndex);
        void addSegment();
        void removeSegment();
        void commitChain();
        void paintSegmentPreview(juce::Graphics& g, juce::Rectangle<int> bounds) const;

        PatchworkEightProcessor& processor_;
        bool embeddedInDesignMode_ = false;
        std::size_t selectedEnv_ = 0;
        std::size_t selectedSegment_ = 0;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        std::array<juce::TextButton, 8> envPills_{};
        SectionPanel chainPanel_{"Segment Chain"};
        std::array<juce::TextButton, pw8::envelope::kMaxSegmentEnvelopeSegments> segmentDots_{};
        juce::TextButton typeChip_{"RMP"};
        juce::TextButton shapeChip_{"LIN"};
        juce::Slider durationSlider_;
        juce::Slider levelSlider_;
        juce::Label durationLabel_;
        juce::Label levelLabel_;
        juce::Slider loopStartSlider_;
        juce::Slider loopEndSlider_;
        juce::Label loopStartLabel_;
        juce::Label loopEndLabel_;
        juce::TextButton addSegmentButton_{"+ SEG"};
        juce::TextButton removeSegmentButton_{"− SEG"};
        juce::TextButton openPlayMotionButton_{"OPEN IN PLAY MOTION"};
        juce::Label footerHint_;

        juce::Rectangle<int> chainPreviewBounds_;
        pw8::envelope::SegmentEnvelopeChain editChain_{};
    };

} // namespace pw8::plugin::ui
