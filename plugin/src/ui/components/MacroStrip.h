#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "../theme/ObsidianPalette.h"

// The 8 macros, laid out as a single row -- Patchwork Eight's actual performance
// surface, and the reason macros were the very first thing this project exposed
// to host automation (docs/PLUGIN_ARCHITECTURE.md "Automation").
//
// Each knob's label starts as the generic "Macro N" (PluginState.h's
// kMacroParameterNames) and switches to the patch-authored name
// (patch::Macro::name -- "Growl", "Brightness", whatever the sound designer
// called it) once a patch with a non-empty name for that macro is loaded. A
// Timer polls for this rather than pushing on patch-load, matching the same
// "no push notification exists yet" reasoning GlowKnob's own modulation-ring
// poll already documents.
namespace pw8::plugin::ui
{
    class MacroStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit MacroStrip(PatchworkEightProcessor& processor);
        ~MacroStrip() override;

        void resized() override;

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Macros", palette::kAccentWarm};
        std::array<std::unique_ptr<GlowKnob>, 8> knobs_;
        std::array<juce::String, 8> lastAppliedNames_; // So the poll only touches a label when its text actually changed.

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroStrip)
    };

} // namespace pw8::plugin::ui
