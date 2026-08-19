#include "EngineGridPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModAssignmentController.h"
#include "pw8/core/Types.hpp"

namespace pw8::plugin::ui
{
    EngineGridPanel::EngineGridPanel(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController)
        : processor_(processor), assignmentController_(assignmentController)
    {
        headerLabel_.setText("ACTIVE PLAY ENGINES", juce::dontSendNotification);
        headerLabel_.setFont(fonts::label(10.0f));
        headerLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        headerLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(headerLabel_);

        polyBadgeLabel_.setFont(fonts::label(8.0f));
        polyBadgeLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        polyBadgeLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(polyBadgeLabel_);

        for (int i = 0; i < 8; ++i)
        {
            cards_[static_cast<std::size_t>(i)] = std::make_unique<EngineCard>(processor_, assignmentController_, i);
            cards_[static_cast<std::size_t>(i)]->setPlayBoardCompactMode(true);
            cards_[static_cast<std::size_t>(i)]->onDoubleClicked = [this](int index) {
                if (onEngineDoubleClicked)
                    onEngineDoubleClicked(index);
            };
            cards_[static_cast<std::size_t>(i)]->onWavetableLabRequested = [this](int index) {
                if (onWavetableLabRequested)
                    onWavetableLabRequested(index);
            };
            addAndMakeVisible(*cards_[static_cast<std::size_t>(i)]);
        }

        startTimerHz(6);
        processor_.ensureDefaultWavetablesForPatch();
        timerCallback();
    }

    EngineGridPanel::~EngineGridPanel() { stopTimer(); }

    void EngineGridPanel::setDesignModeV2Layout(bool designMode)
    {
        if (designModeV2Layout_ == designMode)
            return;

        designModeV2Layout_ = designMode;
        headerLabel_.setVisible(!designMode);
        polyBadgeLabel_.setVisible(!designMode);

        for (auto& card : cards_)
        {
            card->setPlayBoardCompactMode(!designMode);
            card->setDesignModeV2Layout(designMode);
        }

        resized();
        repaint();
    }

    void EngineGridPanel::timerCallback()
    {
        const auto poly = processor_.getCurrentPatch().voiceSettings.polyphony;
        polyBadgeLabel_.setText("POLYPHONY: " + juce::String(static_cast<int>(poly)) + " VOICES",
                                juce::dontSendNotification);
    }

    void EngineGridPanel::paint(juce::Graphics& g)
    {
        if (polyBadgeBounds_.isEmpty())
            return;

        g.setColour(palette::kBorder);
        g.fillRoundedRectangle(polyBadgeBounds_.toFloat(), 4.0f);
    }

    void EngineGridPanel::resized()
    {
        auto bounds = getLocalBounds();

        if (!designModeV2Layout_)
        {
            bounds = bounds.reduced(layout::kEngineGridPadding);
            auto header = bounds.removeFromTop(layout::kEngineGridHeaderHeight);
            headerLabel_.setBounds(header.removeFromLeft(header.getWidth() - layout::kPolyphonyBadgeWidth - 8));
            polyBadgeBounds_ = header.removeFromRight(layout::kPolyphonyBadgeWidth).reduced(0, 3);
            polyBadgeLabel_.setBounds(polyBadgeBounds_);

            bounds.removeFromTop(layout::kEngineGridContentPaddingY);
            bounds.removeFromBottom(layout::kEngineGridContentPaddingY);
        }

        const int cols = 4;
        const int rows = 2;
        const int gap = designModeV2Layout_ ? layout::kDesignModeV2GridColGap : layout::kEngineGridGap;
        const int cellW = (bounds.getWidth() - gap * (cols - 1)) / cols;
        const int cellH = (bounds.getHeight() - gap * (rows - 1)) / rows;

        for (int i = 0; i < 8; ++i)
        {
            const int col = i % cols;
            const int row = i / cols;
            const int x = bounds.getX() + col * (cellW + gap);
            const int y = bounds.getY() + row * (cellH + gap);
            cards_[static_cast<std::size_t>(i)]->setBounds(x, y, cellW, cellH);
        }
    }

} // namespace pw8::plugin::ui
