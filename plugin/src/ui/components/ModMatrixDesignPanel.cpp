#include "ModMatrixDesignPanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kPaletteRowHeight = 34;
        constexpr int kPickerRowHeight = 30;
        constexpr int kConnectionRowHeight = 22;
        constexpr int kRemoveButtonWidth = 16;
        constexpr int kAmountColumnWidth = 72;
    } // namespace

    ModMatrixDesignPanel::ModMatrixDesignPanel(PatchworkEightProcessor& processor) : processor_(processor),
                                                                                      sourcePalette_(assignmentController_)
    {
        destHint_.setFont(fonts::label(fonts::kBodyLabelSize));
        destHint_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(destHint_);

        destCombo_.onChange = [this] { resized(); };
        addAndMakeVisible(destCombo_);

        targetHint_.setFont(fonts::label(fonts::kBodyLabelSize));
        targetHint_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(targetHint_);

        for (int i = 0; i < static_cast<int>(core::kNodesPerLayer); ++i)
            targetCombo_.addItem("Op " + juce::String(i), i + 1);
        targetCombo_.setSelectedId(1, juce::dontSendNotification);
        addAndMakeVisible(targetCombo_);

        addRouteButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        addRouteButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        addRouteButton_.onClick = [this] { addRouteFromSelection(); };
        addAndMakeVisible(addRouteButton_);

        routesHint_.setFont(fonts::label(fonts::kBodyLabelSize));
        routesHint_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(routesHint_);

        addAndMakeVisible(sourcePalette_);

        assignmentController_.onChanged = [this] { sourcePalette_.repaintAssignmentState(); };

        rebuildDestChoices();
        startTimerHz(4);
    }

    void ModMatrixDesignPanel::rebuildDestChoices()
    {
        destChoices_ = {
            {modulation::ModDestination::FilterCutoff, false, "Global Filter Cutoff"},
            {modulation::ModDestination::FilterResonance, false, "Global Filter Resonance"},
            {modulation::ModDestination::OperatorFilterCutoff, true, "Engine Filter Cutoff"},
            {modulation::ModDestination::OperatorFilterResonance, true, "Engine Filter Resonance"},
            {modulation::ModDestination::OperatorLevel, true, "Operator Level"},
            {modulation::ModDestination::OperatorWavetablePosition, true, "WT Position"},
            {modulation::ModDestination::OperatorWavetableBend, true, "WT Bend"},
            {modulation::ModDestination::OperatorWavetableAsymmetry, true, "WT Asymmetry"},
            {modulation::ModDestination::OperatorWavetableSyncRatio, true, "WT Sync Ratio"},
            {modulation::ModDestination::OperatorWavetableSyncAmount, true, "WT Sync Amt"},
            {modulation::ModDestination::OperatorWavetableFormant, true, "WT Formant"},
            {modulation::ModDestination::Pan, false, "Layer Pan"},
        };

        destCombo_.clear(juce::dontSendNotification);
        for (int i = 0; i < static_cast<int>(destChoices_.size()); ++i)
            destCombo_.addItem(destChoices_[static_cast<std::size_t>(i)].label, i + 1);
        if (!destChoices_.empty())
            destCombo_.setSelectedId(1, juce::dontSendNotification);
    }

    void ModMatrixDesignPanel::refreshFromPatch()
    {
        repaint();
    }

    void ModMatrixDesignPanel::addRouteFromSelection()
    {
        const auto source = assignmentController_.armedSource();
        if (!source.has_value())
            return;

        const int destIndex = destCombo_.getSelectedId() - 1;
        if (destIndex < 0 || destIndex >= static_cast<int>(destChoices_.size()))
            return;

        const auto& choice = destChoices_[static_cast<std::size_t>(destIndex)];
        const std::uint8_t targetIndex =
            choice.usesTargetIndex
                ? static_cast<std::uint8_t>(juce::jlimit(0, static_cast<int>(core::kNodesPerLayer) - 1,
                                                         targetCombo_.getSelectedId() - 1))
                : std::uint8_t{0};

        assignModRoute(processor_, *source, choice.destination, targetIndex);
        assignmentController_.disarm();
        sourcePalette_.repaintAssignmentState();
        resized();
        repaint();
    }

    void ModMatrixDesignPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(4);

        sourcePalette_.setBounds(bounds.removeFromTop(kPaletteRowHeight));
        bounds.removeFromTop(6);

        auto pickerRow = bounds.removeFromTop(kPickerRowHeight);
        destHint_.setBounds(pickerRow.removeFromLeft(72));
        destCombo_.setBounds(pickerRow.removeFromLeft(180).reduced(0, 2));
        pickerRow.removeFromLeft(8);

        const int destIndex = destCombo_.getSelectedId() - 1;
        const bool usesTarget =
            destIndex >= 0 && destIndex < static_cast<int>(destChoices_.size()) &&
            destChoices_[static_cast<std::size_t>(destIndex)].usesTargetIndex;
        targetHint_.setVisible(usesTarget);
        targetCombo_.setVisible(usesTarget);
        if (usesTarget)
        {
            targetHint_.setBounds(pickerRow.removeFromLeft(24));
            targetCombo_.setBounds(pickerRow.removeFromLeft(64).reduced(0, 2));
            pickerRow.removeFromLeft(8);
        }

        addRouteButton_.setBounds(pickerRow.removeFromLeft(96).reduced(0, 2));
        bounds.removeFromTop(8);

        routesHint_.setBounds(bounds.removeFromTop(18));
        bounds.removeFromTop(4);

        connectionsArea_ = bounds;
    }

    std::vector<ModMatrixDesignPanel::ConnectionRowLayout> ModMatrixDesignPanel::layoutConnectionRows() const
    {
        std::vector<ConnectionRowLayout> rows;
        auto area = connectionsArea_;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;
            auto row = area.removeFromTop(kConnectionRowHeight);
            auto removeButton = row.removeFromRight(kRemoveButtonWidth);
            auto amountArea = row.removeFromRight(kAmountColumnWidth);
            row.removeFromRight(6);
            rows.push_back({route, row, amountArea, removeButton});
        }
        return rows;
    }

    void ModMatrixDesignPanel::beginAmountDrag(const modulation::ModRoute& route, float startX)
    {
        amountDrag_ = AmountDragState{route, route.amount, startX};
    }

    void ModMatrixDesignPanel::continueAmountDrag(float currentX)
    {
        if (!amountDrag_.has_value())
            return;

        const auto range = modAmountRangeFor(amountDrag_->route.destination);
        const float span = range.max - range.min;
        const float delta = (currentX - amountDrag_->startX) * (span / 120.0f);
        updateModRouteAmount(processor_, amountDrag_->route, amountDrag_->startAmount + delta);
        for (const auto& r : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (r.isActive() && r.source == amountDrag_->route.source &&
                r.destination == amountDrag_->route.destination && r.targetIndex == amountDrag_->route.targetIndex)
            {
                amountDrag_->route.amount = r.amount;
                break;
            }
        }
        repaint();
    }

    void ModMatrixDesignPanel::endAmountDrag()
    {
        amountDrag_.reset();
    }

    void ModMatrixDesignPanel::mouseDown(const juce::MouseEvent& event)
    {
        const auto pos = event.getPosition();
        for (const auto& row : layoutConnectionRows())
        {
            if (row.amountArea.contains(pos))
            {
                beginAmountDrag(row.route, static_cast<float>(event.position.x));
                return;
            }
            if (!row.removeButton.contains(pos))
                continue;
            processor_.removeModRouteLive(row.route.source, row.route.destination, row.route.targetIndex);
            resized();
            repaint();
            return;
        }
    }

    void ModMatrixDesignPanel::mouseDrag(const juce::MouseEvent& event)
    {
        if (amountDrag_.has_value())
            continueAmountDrag(static_cast<float>(event.position.x));
    }

    void ModMatrixDesignPanel::mouseUp(const juce::MouseEvent&)
    {
        endAmountDrag();
    }

    void ModMatrixDesignPanel::timerCallback()
    {
        repaint();
    }

    void ModMatrixDesignPanel::paintOverChildren(juce::Graphics& g)
    {
        const auto rows = layoutConnectionRows();

        if (rows.empty())
        {
            auto area = connectionsArea_.removeFromTop(kConnectionRowHeight * 2);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::value(fonts::kBodyLabelSize));
            g.drawFittedText("No routes yet. Click a source chip, pick a destination, then Add Route.",
                             area, juce::Justification::centredLeft, 2);
            return;
        }

        for (const auto& row : rows)
        {
            const auto colour = palette::modSourceColour(static_cast<int>(row.route.source));

            auto textArea = row.textArea;
            const auto dotBounds = textArea.removeFromLeft(10).withSizeKeepingCentre(6, 6).toFloat();
            g.setColour(colour);
            g.fillEllipse(dotBounds);

            textArea.removeFromLeft(4);
            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::value(fonts::kBodyLabelSize));
            const auto label =
                modSourceLabel(row.route.source) + "  ->  " +
                modDestinationLabel(row.route.destination, row.route.targetIndex);
            g.drawText(label, textArea, juce::Justification::centredLeft);

            const auto range = modAmountRangeFor(row.route.destination);
            const float normalized =
                juce::jlimit(0.0f, 1.0f, (row.route.amount - range.min) / juce::jmax(0.001f, range.max - range.min));
            auto sliderArea = row.amountArea.reduced(2, 5).toFloat();
            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(sliderArea, 2.0f);
            auto fillArea = sliderArea;
            fillArea.setWidth(sliderArea.getWidth() * normalized);
            g.setColour(colour.withAlpha(0.75f));
            g.fillRoundedRectangle(fillArea, 2.0f);

            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::label(fonts::kBodyLabelSize));
            g.drawText(formatModRouteAmount(row.route.destination, row.route.amount), row.amountArea,
                       juce::Justification::centred);

            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(fonts::kBodyLabelSize));
            g.drawText("x", row.removeButton, juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui
