#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"

// Filter 1 and LFO 1's main controls, compact -- the "quick sound-shaping" surface
// PLAY mode offers. The full 8-LFO/8-envelope modulation bank and Filter 2 (once
// it exists) are DESIGN-mode territory (docs/ROADMAP.md GATE 6/Phase 17),
// deliberately out of scope here: PLAY mode is meant to be playable at a glance,
// not a second copy of the flat 578-parameter list.
namespace pw8::plugin::ui
{
    class FilterLfoPanel : public juce::Component
    {
    public:
        explicit FilterLfoPanel(juce::AudioProcessorValueTreeState& apvts);

        void resized() override;

    private:
        SectionPanel filterPanel_{"Filter 1"};
        SectionPanel lfoPanel_{"LFO 1"};

        juce::ToggleButton filterEnabledToggle_{"ENABLED"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> filterEnabledAttachment_;

        std::unique_ptr<GlowKnob> filterMode_;
        std::unique_ptr<GlowKnob> filterCutoff_;
        std::unique_ptr<GlowKnob> filterResonance_;
        std::unique_ptr<GlowKnob> filterKeyTrack_;

        std::unique_ptr<GlowKnob> lfoWaveform_;
        std::unique_ptr<GlowKnob> lfoMode_;
        std::unique_ptr<GlowKnob> lfoRate_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterLfoPanel)
    };

} // namespace pw8::plugin::ui
