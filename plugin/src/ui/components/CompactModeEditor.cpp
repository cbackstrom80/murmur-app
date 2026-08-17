#include "CompactModeEditor.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "InterstellarHudDraw.h"
#include "ModRoutingUi.h"

namespace pw8::plugin::ui
{
    CompactModeEditor::CompactModeEditor(PatchworkEightProcessor& processor)
        : processor_(processor),
          circularScope_(processor),
          focusPanel_(processor)
    {
        for (auto* btn : {&prevButton_, &nextButton_})
        {
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            addAndMakeVisible(*btn);
        }
        prevButton_.onClick = [this] { stepPreset(-1); };
        nextButton_.onClick = [this] { stepPreset(1); };

        missionNameLabel_.setJustificationType(juce::Justification::centredLeft);
        missionNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        missionNameLabel_.setFont(fonts::title(13.0f));
        addAndMakeVisible(missionNameLabel_);

        missionCategoryLabel_.setJustificationType(juce::Justification::centredLeft);
        missionCategoryLabel_.setColour(juce::Label::textColourId, palette::kAccentWarm);
        missionCategoryLabel_.setFont(fonts::label(9.0f));
        addAndMakeVisible(missionCategoryLabel_);

        missionHintLabel_.setJustificationType(juce::Justification::centredLeft);
        missionHintLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        missionHintLabel_.setFont(fonts::value(10.0f));
        addAndMakeVisible(missionHintLabel_);

        addAndMakeVisible(circularScope_);
        circularScope_.setViewMode(processor_.getScopeViewMode());

        focusPanel_.setCompactLayout(true);
        addAndMakeVisible(focusPanel_);

        startTimerHz(2);
        timerCallback();
    }

    CompactModeEditor::~CompactModeEditor()
    {
        stopTimer();
    }

    void CompactModeEditor::updateMissionCard()
    {
        const auto& meta = processor_.getCurrentPatch().metadata;
        const auto name = juce::String(meta.name);
        missionNameLabel_.setText(name.isEmpty() ? "INIT" : name, juce::dontSendNotification);

        juce::String category;
        if (const auto presetPath = processor_.getCurrentPresetPath(); presetPath.isNotEmpty())
            category = juce::File(presetPath).getParentDirectory().getFileName();
        if (category.isEmpty())
            category = "Factory";
        missionCategoryLabel_.setText(category.toUpperCase(), juce::dontSendNotification);

        const auto hint = performanceHintForPatch(processor_.getCurrentPatch(), &processor_.apvts);
        missionHintLabel_.setText(hint, juce::dontSendNotification);
        missionHintLabel_.setVisible(hint.isNotEmpty());

        if (category != lastCategory_ || hint != lastHint_)
        {
            lastCategory_ = category;
            lastHint_ = hint;
            repaint();
        }
    }

    void CompactModeEditor::timerCallback()
    {
        updateMissionCard();
    }

    void CompactModeEditor::stepPreset(int direction)
    {
        if (presetIndex_ == nullptr)
            return;

        const auto current = processor_.getCurrentPresetPath();
        const juce::StringArray* favoritesOnly =
            browseFilter_.favoritesOnly && favoritesStore_ != nullptr ? &favoritesStore_->paths() : nullptr;
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_->nextAfter(current, browseFilter_, favoritesOnly);
        else
            entry = presetIndex_->prevBefore(current, browseFilter_, favoritesOnly);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
        timerCallback();
        focusPanel_.refreshFromPatch();
    }

    void CompactModeEditor::paint(juce::Graphics& g)
    {
        auto card = juce::Rectangle<int>(8, 4, getWidth() - 16, layout::kCompactMissionCardHeight).toFloat();
        g.setColour(palette::kPanel.withAlpha(0.55f));
        g.fillRoundedRectangle(card, 6.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.28f));
        g.drawRoundedRectangle(card, 6.0f, 1.2f);

        if (interstellar::isInterstellarCategory(lastCategory_))
            interstellar::paintHudBadge(g, card.expanded(2.0f), true);
    }

    void CompactModeEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kCompactOuterMargin);

        auto missionCard = bounds.removeFromTop(layout::kCompactMissionCardHeight);
        auto navRow = missionCard.removeFromTop(layout::kCompactHeaderHeight);
        prevButton_.setBounds(navRow.removeFromLeft(26).reduced(1, 4));
        nextButton_.setBounds(navRow.removeFromRight(26).reduced(1, 4));

        auto textCol = navRow.reduced(4, 0);
        missionNameLabel_.setBounds(textCol.removeFromTop(16));
        missionCategoryLabel_.setBounds(textCol.removeFromTop(12));
        if (missionHintLabel_.isVisible())
            missionHintLabel_.setBounds(textCol.removeFromTop(14));

        bounds.removeFromTop(layout::kCompactBlockGap);

        const int scopeSize = juce::jmin(layout::kCompactScopeSize, bounds.getHeight() - 140);
        auto scopeBounds = bounds.removeFromTop(scopeSize).withSizeKeepingCentre(scopeSize, scopeSize);
        circularScope_.setBounds(scopeBounds);

        focusPanel_.setOrbitHole(scopeBounds);
        focusPanel_.setBounds(bounds);
    }

} // namespace pw8::plugin::ui
