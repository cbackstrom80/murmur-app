#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"

// A compact, at-a-glance view of all 3 insert + 4 master FX slots: which
// algorithm each is running (read-only text, live-updating -- type selection is
// a DESIGN-mode editing concern per the master spec's mode split) and the one
// control every non-Bypass algorithm shares, `mix`. Per-algorithm detail editing
// (Reverb's 15 fields, Eq's 7, etc.) is explicitly out of scope for PLAY mode --
// see docs/PLUGIN_ARCHITECTURE.md "Automation" for the full field list, all of
// which remains host-automatable regardless of what PLAY mode chooses to surface.
namespace pw8::plugin::ui
{
    class FxChainStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit FxChainStrip(juce::AudioProcessorValueTreeState& apvts);
        ~FxChainStrip() override;

        void resized() override;

    private:
        void timerCallback() override;

        struct Slot
        {
            juce::Label typeLabel;
            std::unique_ptr<GlowKnob> mixKnob;
            std::atomic<float>* typeParam = nullptr;
        };

        SectionPanel panel_{"FX Chain"};
        std::array<Slot, 7> slots_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxChainStrip)
    };

} // namespace pw8::plugin::ui
