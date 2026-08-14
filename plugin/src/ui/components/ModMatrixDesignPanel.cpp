#include "ModMatrixDesignPanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"
#include "WireframePanel.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kPaletteRowHeight = 34;
        constexpr int kPickerRowHeight = 30;
        constexpr int kConnectionRowHeight = 22;
        constexpr int kRemoveButtonWidth = 16;
        constexpr int kAmountColumnWidth = 72;
        constexpr int kPreviewHeight = 72;
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

        previewArea_ = bounds.removeFromTop(kPreviewHeight);
        bounds.removeFromTop(6);

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

    void ModMatrixDesignPanel::paint(juce::Graphics& g)
    {
        if (previewArea_.isEmpty())
            return;

        WireframePanel::paintFrame(g, previewArea_.toFloat(), "ROUTE MAP", palette::kAccent);
        paintRoutePreview(g, previewArea_.reduced(8, 22));
    }

    void ModMatrixDesignPanel::paintRoutePreview(juce::Graphics& g, juce::Rectangle<int> area) const
    {
        if (area.isEmpty())
            return;

        const auto routeArea = area.toFloat().reduced(4.0f, 2.0f);
        const auto& routes = processor_.getCurrentPatch().layerA.modRoutes;

        struct SourceAnchor
        {
            modulation::ModSource source = modulation::ModSource::None;
            juce::Point<float> pt;
        };
        struct DestAnchor
        {
            modulation::ModDestination destination = modulation::ModDestination::None;
            std::uint8_t targetIndex = 0;
            juce::Point<float> pt;
        };

        std::vector<SourceAnchor> sources;
        std::vector<DestAnchor> destinations;

        auto sourcePt = [&](modulation::ModSource src) -> juce::Point<float> {
            for (const auto& s : sources)
                if (s.source == src)
                    return s.pt;
            const float y = routeArea.getY() + routeArea.getHeight() * (0.18f + static_cast<float>(sources.size()) * 0.22f);
            const auto pt = juce::Point<float>{routeArea.getX() + routeArea.getWidth() * 0.08f, y};
            sources.push_back({src, pt});
            return pt;
        };

        auto destPt = [&](modulation::ModDestination dest, std::uint8_t target) -> juce::Point<float> {
            for (const auto& d : destinations)
            {
                if (d.destination == dest && d.targetIndex == target)
                    return d.pt;
            }
            const float y =
                routeArea.getY() + routeArea.getHeight() * (0.22f + static_cast<float>(destinations.size()) * 0.24f);
            const auto pt = juce::Point<float>{routeArea.getRight() - routeArea.getWidth() * 0.08f, y};
            destinations.push_back({dest, target, pt});
            return pt;
        };

        bool anyActive = false;
        for (const auto& route : routes)
        {
            if (!route.isActive())
                continue;
            anyActive = true;

            const auto from = sourcePt(route.source);
            const auto to = destPt(route.destination, route.targetIndex);
            const auto colour = palette::modSourceColour(static_cast<int>(route.source));

            juce::Path link;
            link.startNewSubPath(from);
            const float cx = (from.x + to.x) * 0.5f;
            link.quadraticTo(cx, from.y - 8.0f, to.x, to.y);

            g.setColour(colour.withAlpha(0.18f));
            g.strokePath(link, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(colour.withAlpha(0.78f));
            g.strokePath(link, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setFont(fonts::label(7.5f));
        for (const auto& s : sources)
        {
            g.setColour(palette::modSourceColour(static_cast<int>(s.source)));
            g.fillEllipse(s.pt.x - 4.0f, s.pt.y - 4.0f, 8.0f, 8.0f);
            g.drawText(modSourceLabel(s.source),
                       juce::Rectangle<float>(s.pt.x - 22.0f, s.pt.y + 5.0f, 44.0f, 10.0f),
                       juce::Justification::centred);
        }

        for (const auto& d : destinations)
        {
            g.setColour(palette::kAccent.withAlpha(0.22f));
            g.fillEllipse(d.pt.x - 5.0f, d.pt.y - 5.0f, 10.0f, 10.0f);
            g.setColour(palette::kAccent);
            g.drawEllipse(d.pt.x - 5.0f, d.pt.y - 5.0f, 10.0f, 10.0f, 1.2f);
            g.drawText(modDestinationLabel(d.destination, d.targetIndex),
                       juce::Rectangle<float>(d.pt.x - 36.0f, d.pt.y + 6.0f, 72.0f, 10.0f),
                       juce::Justification::centred);
        }

        if (!anyActive)
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(9.0f));
            g.drawText("Route preview — add routes below", routeArea, juce::Justification::centred);
        }
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
