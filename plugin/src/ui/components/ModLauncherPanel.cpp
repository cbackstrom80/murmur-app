#include "ModLauncherPanel.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"

namespace pw8::plugin::ui
{
    ModLauncherPanel::ModLauncherPanel(MurmurProcessor& processor,
                                        ModAssignmentController& assignmentController)
        : processor_(processor), modSourceStrip_(processor, assignmentController)
    {
        titleLabel_.setText("Mod Matrix", juce::dontSendNotification);
        titleLabel_.setFont(fonts::title(16.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        summaryLabel_.setFont(fonts::value(11.0f));
        summaryLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        summaryLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(summaryLabel_);

        openButton_.setButtonText("Expand");
        openButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        openButton_.onClick = [this] {
            if (onOpenAdvanced)
                onOpenAdvanced();
        };
        addAndMakeVisible(openButton_);

        addAndMakeVisible(modSourceStrip_);

        startTimerHz(4);
        timerCallback();
    }

    ModLauncherPanel::~ModLauncherPanel()
    {
        stopTimer();
    }

    void ModLauncherPanel::setRoutingContext(FilterPanelScope scope, int engineIndex)
    {
        modSourceStrip_.setRoutingContext(scope, engineIndex);
    }

    void ModLauncherPanel::repaintModAssignmentState()
    {
        modSourceStrip_.repaintModAssignmentState();
    }

    void ModLauncherPanel::timerCallback()
    {
        int routeCount = 0;
        bool hasModWheel = false;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;
            ++routeCount;
            if (route.source == modulation::ModSource::ModWheel)
                hasModWheel = true;
        }

        juce::String summary;
        if (routeCount == 0)
            summary = "Arm a source, pick a destination, drag amount sliders below.";
        else
        {
            summary = juce::String(routeCount) + " route" + (routeCount == 1 ? "" : "s") + " active";
            if (hasModWheel)
                summary += " · Mod Wheel";
            summary += " — edit depths inline.";
        }
        summaryLabel_.setText(summary, juce::dontSendNotification);
        repaint();
    }

    void ModLauncherPanel::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        draw::fillRecessedRoundedRect(g, bounds, 10.0f);
        draw::strokeGlowPath(g, draw::roundedRectPath(bounds, 10.0f), 0.25f, 1.0f, false);
    }

    void ModLauncherPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(12, 10);
        titleLabel_.setBounds(bounds.removeFromTop(22));
        bounds.removeFromTop(2);
        auto headerRow = bounds.removeFromTop(18);
        summaryLabel_.setBounds(headerRow.removeFromLeft(headerRow.getWidth() - 72));
        openButton_.setBounds(headerRow.removeFromRight(68).reduced(0, 1));
        bounds.removeFromTop(4);
        modSourceStrip_.setBounds(bounds);
    }

} // namespace pw8::plugin::ui
