#include "MetadataFacetRow.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::String formatFacetLabel(const juce::String& value)
        {
            const auto trimmed = value.trim();
            if (trimmed.isEmpty())
                return trimmed;
            return trimmed.substring(0, 1).toUpperCase() + trimmed.substring(1).toLowerCase();
        }
    } // namespace

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

    void MetadataFacetRow::setCompactFxMode(bool compact) noexcept
    {
        if (compactFxMode_ == compact)
            return;
        compactFxMode_ = compact;
        label_.setVisible(!compactFxMode_);
        rebuildChips();
        resized();
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
        for (std::size_t i = 0; i < chips_.size(); ++i)
        {
            auto& chip = chips_[i];
            const bool isAll = !compactFxMode_ && i == 0;
            const auto rawValue =
                isAll ? juce::String() : values_[static_cast<int>(compactFxMode_ ? i : i - 1)];
            const bool on = isAll ? selectedValue_.isEmpty() : rawValue.equalsIgnoreCase(selectedValue_);
            chip->setToggleState(on, juce::dontSendNotification);
            if (compactFxMode_)
            {
                chip->setColour(juce::TextButton::buttonOnColourId,
                                on ? palette::kAccent : palette::kFigmaFxVizFill);
                chip->setColour(juce::TextButton::buttonColourId,
                                on ? palette::kFigmaFxToggleOnFill : palette::kFigmaFxVizFill);
                chip->setColour(juce::TextButton::textColourOnId,
                                on ? palette::kTextPrimary : palette::kFigmaFxMutedText);
                chip->setColour(juce::TextButton::textColourOffId, palette::kFigmaFxMutedText);
            }
            else
            {
                chip->setColour(juce::TextButton::buttonOnColourId,
                                on ? palette::kAccent.withAlpha(0.42f) : palette::kPanelRaised);
                chip->setColour(juce::TextButton::buttonColourId,
                                on ? palette::kAccentDim.withAlpha(0.55f) : palette::kPanelRaised);
                chip->setColour(juce::TextButton::textColourOnId,
                                on ? palette::kTextPrimary : palette::kTextSecondary);
                chip->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            }
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

        auto makeChip = [this](const juce::String& displayText, const juce::String& rawValue) {
            auto chip = std::make_unique<juce::TextButton>(displayText);
            chip->setClickingTogglesState(true);
            chip->setRadioGroupId(rowLabel_.hashCode());
            if (compactFxMode_)
            {
                chip->setColour(juce::TextButton::buttonColourId, palette::kFigmaFxVizFill);
                chip->setColour(juce::TextButton::textColourOffId, palette::kFigmaFxMutedText);
            }
            else
            {
                chip->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
                chip->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            }
            chip->onClick = [this, rawValue]() { selectValue(rawValue, true); };
            chipHost_.addAndMakeVisible(*chip);
            chips_.push_back(std::move(chip));
        };

        if (!compactFxMode_)
            makeChip("All", {});
        for (const auto& value : values_)
            makeChip(compactFxMode_ ? value.toUpperCase() : formatFacetLabel(value), value);

        selectValue(selectedValue_, false);
    }

    void MetadataFacetRow::resized()
    {
        auto bounds = getLocalBounds();
        if (!compactFxMode_)
            label_.setBounds(bounds.removeFromLeft(juce::jmin(56, bounds.getWidth() / 5)));
        viewport_.setBounds(bounds);

        const int chipHeight = compactFxMode_ ? 22 : 20;
        const int chipGap = compactFxMode_ ? 6 : 4;
        int x = 0;
        for (auto& chip : chips_)
        {
            const int textW =
                compactFxMode_ ? juce::jmax(36, chip->getButtonText().length() * 6 + 20)
                               : juce::jmax(36, chip->getButtonText().length() * 7 + 14);
            chip->setBounds(x, 0, textW, chipHeight);
            x += textW + chipGap;
        }
        chipHost_.setSize(juce::jmax(x, viewport_.getWidth()), chipHeight);
    }

} // namespace pw8::plugin::ui
