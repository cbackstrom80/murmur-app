#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

// The top strip: current patch name and this synth's own wordmark. Deliberately
// minimal -- prev/next/save preset browsing needs a content-scanning system that
// doesn't exist yet (content/presets/*.pw8 has no runtime index), so this is
// honestly scoped to "shows what's loaded," not a fake browser. PLANNED: real
// prev/next once a preset index exists (docs/ROADMAP.md).
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

        PatchworkEightProcessor& processor_;
        juce::Label patchNameLabel_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowserBar)
    };

} // namespace pw8::plugin::ui
