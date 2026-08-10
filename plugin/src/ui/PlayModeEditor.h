#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/AlgorithmGraphView.h"
#include "components/FilterLfoPanel.h"
#include "components/FxChainStrip.h"
#include "components/MacroStrip.h"
#include "components/PatchBrowserBar.h"
#include "components/SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

// The real editor: replaces `juce::GenericAudioProcessorEditor`
// (docs/PLUGIN_ARCHITECTURE.md "Editor"). One screen, PLAY mode only -- DESIGN
// and LAB modes are PLANNED (docs/ROADMAP.md), not a tab that exists but does
// nothing; there is currently exactly one mode to be in.
//
// Layout, top to bottom: patch name -> the algorithm graph (largest element,
// this synth's actual visual identity) -> Filter 1 + LFO 1 -> the 8 macros (the
// performance surface) -> a compact FX chain strip.
namespace pw8::plugin::ui
{
    class PlayModeEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit PlayModeEditor(PatchworkEightProcessor& processor);
        ~PlayModeEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        PatchworkEightProcessor& processor_;
        ObsidianLookAndFeel lookAndFeel_;

        PatchBrowserBar patchBrowserBar_;
        SectionPanel graphPanel_{"Algorithm"};
        AlgorithmGraphView graphView_;
        FilterLfoPanel filterLfoPanel_;
        MacroStrip macroStrip_;
        FxChainStrip fxChainStrip_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayModeEditor)
    };

} // namespace pw8::plugin::ui
