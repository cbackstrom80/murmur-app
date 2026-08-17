#include "ModSourcePalette.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace pw8::plugin::ui
{
    ModSourcePalette::ModSourcePalette(ModAssignmentController& controller)
        : controller_(controller)
    {
        hintLabel_.setText("Sources:", juce::dontSendNotification);
        hintLabel_.setJustificationType(juce::Justification::centredRight);
        hintLabel_.setFont(fonts::label(fonts::kBodyLabelSize));
        hintLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(hintLabel_);

        viewport_.setViewedComponent(&chipContainer_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);

        std::vector<ChipSpec> specs = {
            {modulation::ModSource::Lfo1, "LFO 1", palette::kModLfo},
            {modulation::ModSource::Lfo2, "LFO 2", palette::kModLfo},
            {modulation::ModSource::Lfo3, "LFO 3", palette::kModLfo},
            {modulation::ModSource::Lfo4, "LFO 4", palette::kModLfo},
            {modulation::ModSource::Env1, "AMP ENV", palette::kModEnv},
            {modulation::ModSource::Env2, "ENV 2", palette::kModEnv},
            {modulation::ModSource::Velocity, "VEL", palette::kModVelocity},
            {modulation::ModSource::ChannelPressure, "AT", palette::kModVelocity},
            {modulation::ModSource::ModWheel, "MW", palette::kModModWheel},
            {modulation::ModSource::Expression, "EXP", palette::kModExpression},
            {modulation::ModSource::Macro1, "M1", palette::kModMacro},
            {modulation::ModSource::Macro2, "M2", palette::kModMacro},
            {modulation::ModSource::Macro3, "M3", palette::kModMacro},
            {modulation::ModSource::Macro4, "M4", palette::kModMacro},
            {modulation::ModSource::Macro5, "M5", palette::kModMacro},
            {modulation::ModSource::Macro6, "M6", palette::kModMacro},
            {modulation::ModSource::Macro7, "M7", palette::kModMacro},
            {modulation::ModSource::Macro8, "M8", palette::kModMacro},
        };

#if JucePlugin_Build_AU
        specs.push_back({modulation::ModSource::Sidechain, "SC", palette::kMurmurViolet});
#endif

        for (const auto& spec : specs)
        {
            auto chip = std::make_unique<ModSourceChip>(controller_, spec.source, spec.label, spec.colour);
            chipContainer_.addAndMakeVisible(*chip);
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

        viewport_.setBounds(bounds);

        if (chips_.empty())
            return;

        const int gap = compact_ ? 4 : 6;
        const int chipWidth = compact_ ? 52 : 56;
        const int totalWidth = chipWidth * static_cast<int>(chips_.size()) + gap * (static_cast<int>(chips_.size()) - 1);
        chipContainer_.setSize(juce::jmax(bounds.getWidth(), totalWidth), bounds.getHeight());

        auto chipBounds = chipContainer_.getLocalBounds();
        for (auto& chip : chips_)
        {
            chip->setBounds(chipBounds.removeFromLeft(chipWidth));
            chipBounds.removeFromLeft(gap);
        }
    }

} // namespace pw8::plugin::ui
