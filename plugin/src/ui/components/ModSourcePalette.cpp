#include "ModSourcePalette.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    ModSourcePalette::ModSourcePalette(ModAssignmentController& controller)
        : controller_(controller)
    {
        hintLabel_.setText("Sources:", juce::dontSendNotification);
        hintLabel_.setJustificationType(juce::Justification::centredRight);
        hintLabel_.setFont(fonts::label(10.0f));
        hintLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(hintLabel_);

        const std::vector<ChipSpec> specs = {
            {modulation::ModSource::Lfo1, "LFO 1", palette::kModLfo},
            {modulation::ModSource::Env1, "AMP ENV", palette::kModEnv},
            {modulation::ModSource::Velocity, "VEL", palette::kModVelocity},
            {modulation::ModSource::Macro1, "M1", palette::kModMacro},
            {modulation::ModSource::Macro2, "M2", palette::kModMacro},
            {modulation::ModSource::Macro3, "M3", palette::kModMacro},
            {modulation::ModSource::Macro4, "M4", palette::kModMacro},
        };

        for (const auto& spec : specs)
        {
            auto chip = std::make_unique<ModSourceChip>(controller_, spec.source, spec.label, spec.colour);
            addAndMakeVisible(*chip);
            chips_.push_back(std::move(chip));
        }
    }

    void ModSourcePalette::setCompactLayout(bool compact)
    {
        compact_ = compact;
        hintLabel_.setVisible(!compact_);
        resized();
    }

    void ModSourcePalette::repaintAssignmentState()
    {
        for (auto& chip : chips_)
            chip->repaint();
    }

    void ModSourcePalette::resized()
    {
        auto bounds = getLocalBounds();
        if (!compact_)
        {
            hintLabel_.setBounds(bounds.removeFromLeft(56));
            bounds.removeFromLeft(4);
        }

        if (chips_.empty())
            return;

        const int gap = compact_ ? 4 : 6;
        const int totalGap = gap * static_cast<int>(chips_.size() - 1);
        const int chipWidth = juce::jmax(56, (bounds.getWidth() - totalGap) / static_cast<int>(chips_.size()));

        for (auto& chip : chips_)
        {
            chip->setBounds(bounds.removeFromLeft(chipWidth));
            bounds.removeFromLeft(gap);
        }
    }

} // namespace pw8::plugin::ui
