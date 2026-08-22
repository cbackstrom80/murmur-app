#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "ConcentricGlowKnob.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "LabLauncherChip.h"
#include "ModAssignmentController.h"
#include "ModSourcePalette.h"
#include "FilterPanelScopeView.h"
#include "SectionPanel.h"
#include "SubAnchorMeter.h"
#include "WireframePanel.h"
#include "processor/MurmurProcessor.h"
#include "wireframe/FilterWireframeView.h"
#include "wireframe/FilterRoutingWireframeView.h"
#include "wireframe/LfoWireframeView.h"

namespace pw8::plugin::ui
{
    enum class FilterPanelScope
    {
        Global,
        Engine,
    };

    /// Global or per-engine filter controls. Global scope includes LFO 1; engine scope
    /// is filter-only and binds to the selected operator's filter APVTS parameters.
    class FilterLfoPanel : public juce::Component
    {
    public:
        explicit FilterLfoPanel(MurmurProcessor& processor, ModAssignmentController& assignmentController);

        void resized() override;
        void paint(juce::Graphics& g) override;

        void setScope(FilterPanelScope scope, int engineIndex = 0);

        /// PLAY dashboard: filter-only strip with launcher to Dual LFO Lab (no inline LFO).
        void setDashboardMode(bool dashboardMode);
        std::function<void()> onLfoLabRequested;

        /// Repaint mod source chips after armed-source changes (from PlayModeEditor).
        void repaintModAssignmentState();

    private:
        void rebuildAttachments();

        MurmurProcessor& processor_;
        ModAssignmentController& assignmentController_;
        FilterPanelScope scope_ = FilterPanelScope::Global;
        int engineIndex_ = 0;
        bool dashboardMode_ = false;

        std::unique_ptr<LabLauncherChip> lfoLabChip_;
        juce::Label filterModeChipLabel_;
        juce::Label filterSlopeChipLabel_;
        juce::Rectangle<int> modeChipBounds_;
        juce::Rectangle<int> slopeChipBounds_;

        SectionPanel filterPanel_{"Global Filter"};
        SectionPanel lfoPanel_{"LFO 1"};
        wireframe::FilterWireframeView filterWireframe_;
        wireframe::LfoWireframeView lfoWireframe_;

        std::unique_ptr<GlowRingButton> filterEnabledButton_;
        juce::Label filterEnabledLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filterEnabledAttachment_;

        std::unique_ptr<GlowKnob> filterMode_;
        std::unique_ptr<ConcentricGlowKnob> filterToneKnob_;
        std::unique_ptr<GlowKnob> filterKeyTrack_;

        std::unique_ptr<GlowKnob> lfoWaveform_;
        std::unique_ptr<GlowKnob> lfoMode_;
        std::unique_ptr<ConcentricGlowKnob> lfoMotionKnob_;

        ModSourcePalette modSourcePalette_;
        SectionPanel filter2Panel_{"Blades Dual Filter"};
        wireframe::FilterRoutingWireframeView filterRoutingWireframe_;
        std::unique_ptr<GlowKnob> filterRouting_;
        std::unique_ptr<GlowKnob> filterModeMorph_;
        std::unique_ptr<GlowRingButton> filter2EnabledButton_;
        juce::Label filter2EnabledLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filter2EnabledAttachment_;
        std::unique_ptr<GlowKnob> filter2Cutoff_;
        std::unique_ptr<GlowKnob> filter2Resonance_;
        std::unique_ptr<GlowKnob> filter2Drive_;
        std::unique_ptr<GlowKnob> filter2CutoffOffset_;
        std::unique_ptr<GlowKnob> filter2KeyTrack_;
        std::unique_ptr<GlowKnob> filter2ModeMorph_;
        WireframePanel scopeFrame_{"SCOPE"};
        FilterPanelScopeView filterScope_;

        // Sub Anchor -- real protected mono-lock sub layer (see
        // pw8/spatial/SubAnchor.hpp). Sits alongside Filter 2 since both
        // operate on Layer A's own signal. Undertow's real approved copper
        // accent (#D4603A, Figma 277:671) tints just this section's
        // controls, via SectionPanel/GlowKnob/GlowRingButton's own real
        // accentColour parameters -- not a new LookAndFeel, not a global
        // reskin.
        SectionPanel subAnchorPanel_{"Sub Anchor", juce::Colour(0xffD4603A)};
        std::unique_ptr<GlowRingButton> subAnchorEnabledButton_;
        juce::Label subAnchorEnabledLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> subAnchorEnabledAttachment_;
        std::unique_ptr<GlowKnob> subAnchorCrossover_;
        std::unique_ptr<GlowKnob> subAnchorMonoAmount_;
        SubAnchorMeter subAnchorMeter_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterLfoPanel)
    };

} // namespace pw8::plugin::ui
