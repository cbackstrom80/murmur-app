#include "ModSourceStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kChipRowHeight = 34;
        constexpr int kHelpRowHeight = 36;
        constexpr int kConnectionRowHeight = 18;
        constexpr int kRemoveButtonWidth = 16;
    } // namespace

    ModSourceStrip::ModSourceStrip(PatchworkEightProcessor& processor)
        : processor_(processor), routingWireframe_(processor, processor.apvts)
    {
        addAndMakeVisible(panel_);
        panel_.setInterceptsMouseClicks(false, true);
        panel_.addAndMakeVisible(routingWireframe_);

        helpLabel_.setText("1) Drag a source chip onto a ringed knob on the FILTER tab (Cutoff / Resonance). "
                           "2) Active routes appear below. Click x to remove.",
                           juce::dontSendNotification);
        helpLabel_.setJustificationType(juce::Justification::centredLeft);
        helpLabel_.setFont(fonts::value(10.0f));
        helpLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addAndMakeVisible(helpLabel_);

        chips_[0] = std::make_unique<ModSourceChip>(modulation::ModSource::Lfo1, "LFO 1", palette::kModLfo);
        chips_[1] = std::make_unique<ModSourceChip>(modulation::ModSource::Env1, "AMP ENV", palette::kModEnv);
        chips_[2] = std::make_unique<ModSourceChip>(modulation::ModSource::Velocity, "VELOCITY", palette::kModVelocity);
        for (auto& chip : chips_)
            panel_.addAndMakeVisible(*chip);

        startTimerHz(8); // Matches GlowKnob's own mod-route poll rate -- no push notification exists yet.
    }

    ModSourceStrip::~ModSourceStrip()
    {
        stopTimer();
    }

    void ModSourceStrip::timerCallback()
    {
        if (!hasEverHadModRoute_ && processor_.hasUserCreatedModRouteLive())
            hasEverHadModRoute_ = true;
        helpLabel_.setVisible(!hasEverHadModRoute_);
        repaint(); // Cheap: at most a few dozen small ModRoute structs, no allocation.
    }

    void ModSourceStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto bounds = panel_.getContentBounds();

        auto wireBounds = bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.42f)).reduced(0, 2);
        routingWireframe_.setBounds(wireBounds);
        bounds = bounds.reduced(6, 0);

        if (helpLabel_.isVisible())
        {
            helpLabel_.setBounds(bounds.removeFromTop(kHelpRowHeight));
            bounds.removeFromTop(4);
        }

        auto chipRow = bounds.removeFromTop(kChipRowHeight);
        constexpr int kChipWidth = 108;
        constexpr int kGap = 8;
        for (auto& chip : chips_)
        {
            chip->setBounds(chipRow.removeFromLeft(kChipWidth));
            chipRow.removeFromLeft(kGap);
        }
    }

    juce::Rectangle<int> ModSourceStrip::connectionsAreaBounds() const
    {
        auto bounds = panel_.getContentBounds();
        bounds.removeFromTop(kChipRowHeight + 4);
        if (helpLabel_.isVisible())
            bounds.removeFromTop(kHelpRowHeight + 4);
        return bounds;
    }

    std::vector<ModSourceStrip::ConnectionRowLayout> ModSourceStrip::layoutConnectionRows() const
    {
        std::vector<ConnectionRowLayout> rows;
        auto area = connectionsAreaBounds();
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;
            auto row = area.removeFromTop(kConnectionRowHeight);
            auto removeButton = row.removeFromRight(kRemoveButtonWidth);
            rows.push_back({route, row, removeButton});
        }
        return rows;
    }

    void ModSourceStrip::mouseDown(const juce::MouseEvent& event)
    {
        const auto pos = event.getPosition();
        for (const auto& row : layoutConnectionRows())
        {
            if (!row.removeButton.contains(pos))
                continue;
            processor_.removeModRouteLive(row.route.source, row.route.destination, row.route.targetIndex);
            repaint();
            return;
        }
    }

    void ModSourceStrip::paintOverChildren(juce::Graphics& g)
    {
        const auto rows = layoutConnectionRows();

        if (rows.empty())
        {
            auto area = connectionsAreaBounds().removeFromTop(kConnectionRowHeight * 2);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(10.5f));
            g.drawFittedText("No routes yet. Example: drag LFO 1 onto Global Filter Cutoff on the FILTER tab, "
                             "then tweak LFO Rate to hear the sweep.",
                             area, juce::Justification::centredLeft, 2);
            return;
        }

        for (const auto& row : rows)
        {
            const auto colour = palette::modSourceColour(static_cast<int>(row.route.source));

            auto textArea = row.textArea;
            const auto dotBounds =
                textArea.removeFromLeft(10).withSizeKeepingCentre(6, 6).toFloat();
            g.setColour(colour);
            g.fillEllipse(dotBounds);

            textArea.removeFromLeft(4);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::value(10.5f));
            const auto label =
                modSourceLabel(row.route.source) + "  ->  " + modDestinationLabel(row.route.destination, row.route.targetIndex);
            g.drawText(label, textArea, juce::Justification::centredLeft);

            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(12.0f));
            g.drawText("x", row.removeButton, juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui
