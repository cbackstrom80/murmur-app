#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"

// The 8 macros, laid out as a single row -- Patchwork Eight's actual performance
// surface, and the reason macros were the very first thing this project exposed
// to host automation (docs/PLUGIN_ARCHITECTURE.md "Automation").
namespace pw8::plugin::ui
{
    class MacroStrip : public juce::Component
    {
    public:
        explicit MacroStrip(juce::AudioProcessorValueTreeState& apvts);

        void resized() override;

    private:
        SectionPanel panel_{"Macros"};
        std::array<std::unique_ptr<GlowKnob>, 8> knobs_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroStrip)
    };

} // namespace pw8::plugin::ui
