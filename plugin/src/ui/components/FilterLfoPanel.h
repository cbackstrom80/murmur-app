#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "ModAssignmentController.h"
#include "ModSourcePalette.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/FilterWireframeView.h"
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
        explicit FilterLfoPanel(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController);

        void resized() override;
        void paint(juce::Graphics& g) override;

        void setScope(FilterPanelScope scope, int engineIndex = 0);

        /// Repaint mod source chips after armed-source changes (from PlayModeEditor).
        void repaintModAssignmentState();

    private:
        void rebuildAttachments();

        PatchworkEightProcessor& processor_;
        ModAssignmentController& assignmentController_;
        FilterPanelScope scope_ = FilterPanelScope::Global;
        int engineIndex_ = 0;

        SectionPanel filterPanel_{"Global Filter"};
        SectionPanel lfoPanel_{"LFO 1"};
        wireframe::FilterWireframeView filterWireframe_;
        wireframe::LfoWireframeView lfoWireframe_;

        std::unique_ptr<GlowRingButton> filterEnabledButton_;
        juce::Label filterEnabledLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filterEnabledAttachment_;

        std::unique_ptr<GlowKnob> filterMode_;
        std::unique_ptr<GlowKnob> filterCutoff_;
        std::unique_ptr<GlowKnob> filterResonance_;
        std::unique_ptr<GlowKnob> filterKeyTrack_;

        std::unique_ptr<GlowKnob> lfoWaveform_;
        std::unique_ptr<GlowKnob> lfoMode_;
        std::unique_ptr<GlowKnob> lfoRate_;

        ModSourcePalette modSourcePalette_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterLfoPanel)
    };

} // namespace pw8::plugin::ui
