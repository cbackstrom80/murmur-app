#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "ModSourceStrip.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Full-screen overlay for advanced modulation routing (sources, destinations, route list).
    class ModRoutingOverlay : public juce::Component
    {
    public:
        ModRoutingOverlay(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController);

        std::function<void()> onClosed;

        void showOverlay();
        void dismiss();

        void setRoutingContext(FilterPanelScope scope, int engineIndex);

        /// Repaint mod source chips after armed-source changes.
        void repaintModAssignmentState();

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        juce::Component panel_;
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::TextButton closeButton_{"CLOSE"};
        ModSourceStrip modSourceStrip_;
    };

} // namespace pw8::plugin::ui
