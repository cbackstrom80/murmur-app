#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ConcentricGlowKnob.h"
#include "GlowKnob.h"
#include "MasterEnvelopePanel.h"
#include "MorphTimelineStrip.h"
#include "SectionPanel.h"
#include "../PlayModeLayout.h"
#include "processor/MurmurProcessor.h"
#include "wireframe/LfoWireframeView.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-master-motion-lab` (`94:4715`) — env0 hero + morph KOIN + LFO 1–4 grid (Ben 1.4.4).
    class MasterMotionLabPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit MasterMotionLabPanel(MurmurProcessor& processor);
        ~MasterMotionLabPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenModMatrix;
        std::function<void()> onOpenMorphEditor;
        std::function<void()> onOpenEnvelopeSegments;

        void showOverlay();
        void dismiss();

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        struct LfoColumn
        {
            SectionPanel panel;
            wireframe::LfoWireframeView wireframe;
            std::unique_ptr<GlowKnob> waveform;
            std::unique_ptr<GlowKnob> mode;
            std::unique_ptr<ConcentricGlowKnob> motion;
            juce::Label routingLabel;
            juce::Label routingLegend;
            juce::Label syncHeaderLabel;
            std::array<juce::TextButton, 7> syncButtons{};
            juce::Colour accent;
            std::size_t lfoIndex = 0;

            LfoColumn(MurmurProcessor& processor, std::size_t lfoIndex, const char* title, juce::Colour accent);
            void layoutColumn();
        };

        void timerCallback() override;
        void highlightSyncButtons(LfoColumn& column);

        MurmurProcessor& processor_;
        juce::TextButton backButton_{"← PLAY BOARD"};
        juce::TextButton closeButton_{"×"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::Label envelopeSectionLabel_;
        juce::Label morphSectionLabel_;
        juce::Label lfoSectionLabel_;
        MorphTimelineStrip morphTimeline_;
        MasterEnvelopePanel envelopePanel_;
        LfoColumn lfo1_;
        LfoColumn lfo2_;
        LfoColumn lfo3_;
        LfoColumn lfo4_;
        juce::TextButton openModMatrixButton_{"OPEN MOD MATRIX"};
        juce::TextButton openMorphEditorButton_{"MORPH EDITOR"};
        juce::TextButton openSegmentsButton_{"SEGMENTS"};
        juce::Label footerHint_;
    };

} // namespace pw8::plugin::ui
