#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    [[nodiscard]] juce::String modSourceDragDescription(modulation::ModSource source);

    [[nodiscard]] std::optional<modulation::ModSource> parseModSourceDragDescription(const juce::String& description);

    [[nodiscard]] juce::String modSourceLabel(modulation::ModSource source);
    [[nodiscard]] juce::String modDestinationLabel(modulation::ModDestination destination, std::uint8_t targetIndex);

    class ModSourceChip : public juce::Component
    {
    public:
        ModSourceChip(ModAssignmentController& controller, modulation::ModSource source, const juce::String& label,
                      juce::Colour colour);

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;

    private:
        ModAssignmentController& controller_;
        modulation::ModSource source_;
        juce::String label_;
        juce::Colour colour_;
        bool dragStarted_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceChip)
    };

} // namespace pw8::plugin::ui
