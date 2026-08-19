#include "ArpLauncherChip.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ArpUiFormatters.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    ArpLauncherChip::ArpLauncherChip(MurmurProcessor& processor)
        : processor_(processor)
    {
        enableButton_.setAccentColour(palette::kAccent);
        enableButton_.onClick = [this] {
            repaint();
            if (enableButton_.getToggleState() && onOpenDrawer)
                onOpenDrawer();
        };
        addAndMakeVisible(enableButton_);

        rateLabel_.setJustificationType(juce::Justification::centredLeft);
        rateLabel_.setFont(fonts::value(10.5f));
        rateLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(rateLabel_);

        enableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, juce::String(kArpIdPrefix) + "Enabled", enableButton_);

        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setHelpText("Toggle arpeggiator or click to open settings (A)");
        startTimerHz(8);
        timerCallback();
    }

    ArpLauncherChip::~ArpLauncherChip()
    {
        stopTimer();
    }

    juce::String ArpLauncherChip::rateReadout() const
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        return arpRateSummary(arp.rateMode == sequencer::ArpRateMode::TempoSync, arp.rateHz, arp.syncDivisionIndex);
    }

    void ArpLauncherChip::timerCallback()
    {
        pulseOn_ = !pulseOn_;
        const auto text = rateReadout();
        if (rateLabel_.getText() != text)
            rateLabel_.setText(text, juce::dontSendNotification);
        repaint();
    }

    void ArpLauncherChip::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        draw::fillRecessedRoundedRect(g, bounds, bounds.getHeight() * 0.5f);

        const bool enabled = enableButton_.getToggleState();
        if (enabled)
        {
            const float alpha = pulseOn_ ? 0.55f : 0.28f;
            draw::strokeGlowPath(g, draw::roundedRectPath(bounds.reduced(1.0f), bounds.getHeight() * 0.5f), alpha,
                                   1.4f, true);
        }

        g.setColour(enabled ? palette::kAccent : palette::kBorderBright);
        g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() * 0.5f, 1.0f);
    }

    void ArpLauncherChip::resized()
    {
        auto bounds = getLocalBounds().reduced(4, 3);
        enableButton_.setBounds(bounds.removeFromLeft(44));
        bounds.removeFromLeft(6);
        rateLabel_.setBounds(bounds);
    }

    void ArpLauncherChip::mouseUp(const juce::MouseEvent& event)
    {
        if (event.mouseWasClicked() && !enableButton_.getBounds().contains(event.getPosition()) && onOpenDrawer)
            onOpenDrawer();
    }

} // namespace pw8::plugin::ui
