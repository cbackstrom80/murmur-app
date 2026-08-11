#include "ModSourceStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kChipRowHeight = 34;
        constexpr int kConnectionRowHeight = 18;
        constexpr int kRemoveButtonWidth = 16;
    } // namespace

    ModSourceStrip::ModSourceStrip(PatchworkEightProcessor& processor) : processor_(processor)
    {
        addAndMakeVisible(panel_);
        panel_.setInterceptsMouseClicks(false, true); // See OperatorEditorPanel -- lets our own mouseDown see clicks.
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
        repaint(); // Cheap: at most a few dozen small ModRoute structs, no allocation.
    }

    void ModSourceStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto bounds = panel_.getContentBounds();
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
            auto area = connectionsAreaBounds().removeFromTop(kConnectionRowHeight);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(10.5f));
            g.drawText("No active modulation routes -- drag a source above onto a ringed knob.", area,
                       juce::Justification::centredLeft);
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
