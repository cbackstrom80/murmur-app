#include "MetadataFacetRow.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    MetadataFacetRow::MetadataFacetRow(const juce::String& rowLabel) : rowLabel_(rowLabel)
    {
        label_.setText(rowLabel, juce::dontSendNotification);
        label_.setFont(fonts::label(fonts::kBodyLabelSize));
        label_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        label_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label_);

        viewport_.setViewedComponent(&chipHost_, false);
        viewport_.setScrollBarsShown(false, true);
        viewport_.setScrollBarThickness(6);
        addAndMakeVisible(viewport_);
    }

    void MetadataFacetRow::setValues(const juce::StringArray& values)
    {
        if (values_ == values)
            return;
        values_ = values;
        rebuildChips();
        resized();
    }

    void MetadataFacetRow::setSelectedValue(const juce::String& value)
    {
        selectValue(value, false);
    }

    void MetadataFacetRow::selectValue(const juce::String& value, bool notify)
    {
        selectedValue_ = value;
        for (auto& chip : chips_)
        {
            const bool isAll = chip->getButtonText() == "All";
            const bool on = isAll ? selectedValue_.isEmpty() : chip->getButtonText().equalsIgnoreCase(selectedValue_);
            chip->setToggleState(on, juce::dontSendNotification);
            chip->setColour(juce::TextButton::buttonOnColourId,
                            on ? palette::kAccent.withAlpha(0.42f) : palette::kPanelRaised);
            chip->setColour(juce::TextButton::buttonColourId,
                            on ? palette::kAccentDim.withAlpha(0.55f) : palette::kPanelRaised);
            chip->setColour(juce::TextButton::textColourOnId,
                            on ? palette::kTextPrimary : palette::kTextSecondary);
            chip->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        }
        if (notify && onChange)
        {
            // Defer: rebuildFacetsAndList() destroys chip buttons — never run that
            // synchronously from inside a chip's onClick handler.
            const auto callback = onChange;
            juce::MessageManager::callAsync([callback] { callback(); });
        }
    }

    void MetadataFacetRow::rebuildChips()
    {
        chips_.clear();
        chipHost_.removeAllChildren();

        auto makeChip = [this](const juce::String& text) {
            auto chip = std::make_unique<juce::TextButton>(text);
            chip->setClickingTogglesState(true);
            chip->setRadioGroupId(rowLabel_.hashCode());
            chip->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            chip->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            chip->onClick = [this, text]() { selectValue(text == "All" ? juce::String() : text, true); };
            chipHost_.addAndMakeVisible(*chip);
            chips_.push_back(std::move(chip));
        };

        makeChip("All");
        for (const auto& value : values_)
            makeChip(value);

        selectValue(selectedValue_, false);
    }

    void MetadataFacetRow::resized()
    {
        auto bounds = getLocalBounds();
        label_.setBounds(bounds.removeFromLeft(juce::jmin(56, bounds.getWidth() / 5)));
        viewport_.setBounds(bounds);

        constexpr int kChipHeight = 20;
        constexpr int kChipGap = 4;
        int x = 0;
        for (auto& chip : chips_)
        {
            const int textW = juce::jmax(36, chip->getButtonText().length() * 7 + 14);
            chip->setBounds(x, 0, textW, kChipHeight);
            x += textW + kChipGap;
        }
        chipHost_.setSize(juce::jmax(x, viewport_.getWidth()), kChipHeight);
    }

} // namespace pw8::plugin::ui
