#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "ModSourceStrip.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// MOD tab: route summary + embedded editable matrix (Phase 3) + gateway to full-screen overlay.
    class ModLauncherPanel : public juce::Component, private juce::Timer
    {
    public:
        ModLauncherPanel(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController);

        ~ModLauncherPanel() override;

        void resized() override;
        void paint(juce::Graphics& g) override;

        void setRoutingContext(FilterPanelScope scope, int engineIndex);
        void repaintModAssignmentState();

        std::function<void()> onOpenAdvanced;

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        juce::Label titleLabel_;
        juce::Label summaryLabel_;
        juce::TextButton openButton_{"Full screen..."};
        ModSourceStrip modSourceStrip_;
    };

} // namespace pw8::plugin::ui
