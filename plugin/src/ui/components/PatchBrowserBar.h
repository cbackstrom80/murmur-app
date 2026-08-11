#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

// The top strip: current patch name, this synth's own wordmark, and a real
// "Load..." button. Deliberately minimal otherwise -- prev/next/save preset
// BROWSING needs a content-scanning system that doesn't exist yet
// (content/presets/*.pw8 has no runtime index), so this stays honestly scoped
// to "shows what's loaded, and can load one file you pick" rather than a fake
// browser. PLANNED: real prev/next once a preset index exists (docs/ROADMAP.md).
//
// "Load..." exists specifically because there was previously NO way to get a
// saved .pw8 into a running plugin instance short of a debug env-var hack --
// real friction the first time this plugin was used inside an actual DAW
// (REAPER) rather than the Standalone app. Opens a native file chooser, reads
// the chosen file's raw bytes, and calls setStateInformation() directly --
// the exact same path a host uses to restore a saved session, so "load a
// patch by hand" and "reopen a saved project" are provably the same code.
namespace pw8::plugin::ui
{
    class PatchBrowserBar : public juce::Component, private juce::Timer
    {
    public:
        explicit PatchBrowserBar(PatchworkEightProcessor& processor);
        ~PatchBrowserBar() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void loadPatchFromFile();

        PatchworkEightProcessor& processor_;
        juce::Label patchNameLabel_;
        juce::TextButton loadButton_{"LOAD..."};
        std::unique_ptr<juce::FileChooser> fileChooser_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowserBar)
    };

} // namespace pw8::plugin::ui
