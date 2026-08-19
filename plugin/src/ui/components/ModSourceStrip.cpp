#include "ModSourceStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kPaletteRowHeight = 34;
        constexpr int kDestinationRowHeight = 30;
        constexpr int kHelpRowHeight = 40;
        constexpr int kConnectionRowHeight = 22;
        constexpr int kRemoveButtonWidth = 16;
        constexpr int kAmountColumnWidth = 72;
        constexpr float kWireframeColumnRatio = 0.42f;
    } // namespace

    ModSourceStrip::ModSourceStrip(MurmurProcessor& processor, ModAssignmentController& assignmentController)
        : processor_(processor),
          assignmentController_(assignmentController),
          routingWireframe_(processor, processor.apvts),
          sourcePalette_(assignmentController)
    {
        addAndMakeVisible(panel_);
        panel_.setInterceptsMouseClicks(false, true);
        panel_.addAndMakeVisible(routingWireframe_);

        helpLabel_.setText("① Click a source chip  →  ② Click Cutoff or Resonance (or a ringed knob on FILTER). "
                           "Drag chips onto ringed knobs. Right-click rings to remove.",
                           juce::dontSendNotification);
        helpLabel_.setJustificationType(juce::Justification::centredLeft);
        helpLabel_.setFont(fonts::value(fonts::kBodyLabelSize));
        helpLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        panel_.addAndMakeVisible(helpLabel_);

        panel_.addAndMakeVisible(sourcePalette_);

        destinationLabel_.setText("Route to:", juce::dontSendNotification);
        destinationLabel_.setJustificationType(juce::Justification::centredRight);
        destinationLabel_.setFont(fonts::label(fonts::kBodyLabelSize));
        destinationLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        panel_.addAndMakeVisible(destinationLabel_);

        for (auto* button : {&cutoffButton_, &resonanceButton_})
        {
            button->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            button->setColour(juce::TextButton::textColourOffId, palette::kTextPrimary);
            panel_.addAndMakeVisible(*button);
        }

        cutoffButton_.onClick = [this] { assignToCutoff(); };
        resonanceButton_.onClick = [this] { assignToResonance(); };
        refreshDestinationButtonLabels();

        startTimerHz(8);
    }

    ModSourceStrip::~ModSourceStrip()
    {
        stopTimer();
    }

    void ModSourceStrip::setRoutingContext(FilterPanelScope scope, int engineIndex)
    {
        routingScope_ = scope;
        routingEngineIndex_ = engineIndex;
        refreshDestinationButtonLabels();
    }

    void ModSourceStrip::repaintModAssignmentState()
    {
        sourcePalette_.repaintAssignmentState();
    }

    void ModSourceStrip::refreshDestinationButtonLabels()
    {
        if (routingScope_ == FilterPanelScope::Global)
        {
            cutoffButton_.setButtonText("Global Cutoff");
            resonanceButton_.setButtonText("Global Resonance");
        }
        else
        {
            const auto eng = juce::String(routingEngineIndex_);
            cutoffButton_.setButtonText("Eng " + eng + " Cutoff");
            resonanceButton_.setButtonText("Eng " + eng + " Resonance");
        }
    }

    void ModSourceStrip::assignToCutoff()
    {
        const auto source = assignmentController_.armedSource();
        if (!source.has_value())
            return;

        if (routingScope_ == FilterPanelScope::Global)
            assignModRoute(processor_, *source, modulation::ModDestination::FilterCutoff, 0);
        else
            assignModRoute(processor_, *source, modulation::ModDestination::OperatorFilterCutoff,
                           static_cast<std::uint8_t>(routingEngineIndex_));

        assignmentController_.disarm();
        repaint();
    }

    void ModSourceStrip::assignToResonance()
    {
        const auto source = assignmentController_.armedSource();
        if (!source.has_value())
            return;

        if (routingScope_ == FilterPanelScope::Global)
            assignModRoute(processor_, *source, modulation::ModDestination::FilterResonance, 0);
        else
            assignModRoute(processor_, *source, modulation::ModDestination::OperatorFilterResonance,
                           static_cast<std::uint8_t>(routingEngineIndex_));

        assignmentController_.disarm();
        repaint();
    }

    void ModSourceStrip::timerCallback()
    {
        if (!hasEverHadModRoute_)
        {
            for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
            {
                if (route.isActive())
                {
                    hasEverHadModRoute_ = true;
                    break;
                }
            }
        }
        if (!hasEverHadModRoute_ && processor_.hasUserCreatedModRouteLive())
            hasEverHadModRoute_ = true;
        helpLabel_.setVisible(!hasEverHadModRoute_);

        const bool armed = assignmentController_.isArmed();
        cutoffButton_.setEnabled(armed);
        resonanceButton_.setEnabled(armed);

        repaint();
    }

    void ModSourceStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto bounds = panel_.getContentBounds();

        auto wireBounds = bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * kWireframeColumnRatio)).reduced(0, 2);
        routingWireframe_.setBounds(wireBounds);
        rightColumnBounds_ = bounds.reduced(6, 0);

        auto rightLayout = rightColumnBounds_;
        if (helpLabel_.isVisible())
        {
            helpLabel_.setBounds(rightLayout.removeFromTop(kHelpRowHeight));
            rightLayout.removeFromTop(4);
        }

        sourcePalette_.setBounds(rightLayout.removeFromTop(kPaletteRowHeight));
        rightLayout.removeFromTop(6);

        auto destinationRow = rightLayout.removeFromTop(kDestinationRowHeight);
        destinationLabel_.setBounds(destinationRow.removeFromLeft(56));
        destinationRow.removeFromLeft(4);
        cutoffButton_.setBounds(destinationRow.removeFromLeft(132).reduced(0, 2));
        destinationRow.removeFromLeft(8);
        resonanceButton_.setBounds(destinationRow.removeFromLeft(132).reduced(0, 2));

        rightLayout.removeFromTop(8);
        connectionsArea_ = rightLayout;
    }

    juce::Rectangle<int> ModSourceStrip::connectionsAreaBounds() const
    {
        return connectionsArea_.isEmpty() ? rightColumnBounds_ : connectionsArea_;
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
            auto amountArea = row.removeFromRight(kAmountColumnWidth);
            row.removeFromRight(6);
            rows.push_back({route, row, amountArea, removeButton});
        }
        return rows;
    }

    void ModSourceStrip::beginAmountDrag(const modulation::ModRoute& route, float startX)
    {
        amountDrag_ = AmountDragState{route, route.amount, startX};
    }

    void ModSourceStrip::continueAmountDrag(float currentX)
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

    void ModSourceStrip::endAmountDrag()
    {
        amountDrag_.reset();
    }

    void ModSourceStrip::mouseDown(const juce::MouseEvent& event)
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
            repaint();
            return;
        }
    }

    void ModSourceStrip::mouseDrag(const juce::MouseEvent& event)
    {
        if (amountDrag_.has_value())
            continueAmountDrag(static_cast<float>(event.position.x));
    }

    void ModSourceStrip::mouseUp(const juce::MouseEvent&)
    {
        endAmountDrag();
    }

    void ModSourceStrip::paintOverChildren(juce::Graphics& g)
    {
        const auto rows = layoutConnectionRows();

        if (rows.empty())
        {
            auto area = connectionsAreaBounds().removeFromTop(kConnectionRowHeight * 2);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::value(fonts::kBodyLabelSize));
            g.drawFittedText("No routes yet. Try: click LFO 1, then Global Cutoff, then tweak LFO Rate on FILTER.",
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
            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::value(fonts::kBodyLabelSize));
            const auto label =
                modSourceLabel(row.route.source) + "  ->  " + modDestinationLabel(row.route.destination, row.route.targetIndex);
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
