#pragma once

#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    /// Horizontal row of PLAY-mode mod source chips (LFO 1, AMP ENV, Velocity, Macro 1–4).
    /// Used on both MOD and FILTER tabs so routing never requires a cross-tab drag.
    class ModSourcePalette : public juce::Component
    {
    public:
        explicit ModSourcePalette(ModAssignmentController& controller);

        void resized() override;

        /// When true, hides the leading hint label (FILTER tab embed).
        void setCompactLayout(bool compact);

        /// Repaint chips when another source is armed/disarmed.
        void repaintAssignmentState();

    private:
        struct ChipSpec
        {
            modulation::ModSource source;
            juce::String label;
            juce::Colour colour;
        };

        ModAssignmentController& controller_;
        juce::Label hintLabel_;
        std::vector<std::unique_ptr<ModSourceChip>> chips_;
        bool compact_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourcePalette)
    };

} // namespace pw8::plugin::ui
