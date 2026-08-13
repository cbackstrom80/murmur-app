#include "ArpPanelOverlay.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ArpUiFormatters.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    ArpPanelOverlay::ArpPanelOverlay(PatchworkEightProcessor& processor)
        : stepStrip_(processor)
    {
        setVisible(false);
        addAndMakeVisible(drawer_);
        drawer_.addAndMakeVisible(titleLabel_);
        drawer_.addAndMakeVisible(subtitleLabel_);
        drawer_.addAndMakeVisible(closeButton_);
        drawer_.addAndMakeVisible(enableButton_);
        drawer_.addAndMakeVisible(stepStrip_);

        titleLabel_.setText("Arpeggiator", juce::dontSendNotification);
        titleLabel_.setFont(fonts::title(16.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        titleLabel_.setJustificationType(juce::Justification::centredLeft);

        subtitleLabel_.setText("Pattern timing and chord order — click steps to toggle rest. Esc to close.",
                               juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::value(10.5f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        subtitleLabel_.setJustificationType(juce::Justification::centredLeft);

        closeButton_.onClick = [this] { dismiss(); };
        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);

        enableButton_.setAccentColour(palette::kAccent);
        enableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.apvts, juce::String(kArpIdPrefix) + "Enabled", enableButton_);

        auto& apvts = processor.apvts;
        modeKnob_ = std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "Mode", "Mode", arpModeToText);
        rateModeKnob_ =
            std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "RateMode", "Rate Mode", arpRateModeToText);
        rateHzKnob_ = std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "RateHz", "Rate Hz");
        syncDivisionKnob_ = std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "SyncDivisionIndex",
                                                        "Division", arpSyncDivisionToText);
        octaveRangeKnob_ =
            std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "OctaveRange", "Octaves");
        numStepsKnob_ = std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "NumSteps", "Steps");
        latchKnob_ = std::make_unique<GlowKnob>(apvts, juce::String(kArpIdPrefix) + "Latch", "Latch",
                                                 [](float v) { return v >= 0.5f ? "ON" : "OFF"; });

        for (auto* knob :
             {modeKnob_.get(), rateModeKnob_.get(), rateHzKnob_.get(), syncDivisionKnob_.get(), octaveRangeKnob_.get(),
              numStepsKnob_.get(), latchKnob_.get()})
            drawer_.addAndMakeVisible(*knob);
    }

    void ArpPanelOverlay::showDrawer()
    {
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        drawer_.setVisible(true);
        resized();
        grabKeyboardFocus();
        toFront(true);
    }

    void ArpPanelOverlay::dismiss()
    {
        setVisible(false);
        setInterceptsMouseClicks(false, true);
        if (onClosed)
            onClosed();
    }

    void ArpPanelOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop.withAlpha(0.4f));

        auto drawerBounds = drawer_.getBounds().toFloat();
        draw::fillRecessedRoundedRect(g, drawerBounds, 8.0f);
        draw::strokeGlowPath(g, draw::roundedRectPath(drawerBounds, 8.0f), 0.45f, 1.2f, false);
    }

    void ArpPanelOverlay::resized()
    {
        auto bounds = getLocalBounds();
        drawer_.setBounds(bounds.removeFromRight(layout::kArpDrawerWidth));

        auto content = drawer_.getLocalBounds().reduced(14);
        auto header = content.removeFromTop(24);
        titleLabel_.setBounds(header.removeFromLeft(header.getWidth() - 76));
        closeButton_.setBounds(header.removeFromRight(72));
        content.removeFromTop(4);
        subtitleLabel_.setBounds(content.removeFromTop(36));
        content.removeFromTop(8);

        auto enableRow = content.removeFromTop(32);
        enableButton_.setBounds(enableRow.removeFromLeft(44));
        content.removeFromTop(8);

        const int knobWidth = content.getWidth() / 3;
        auto row1 = content.removeFromTop(108);
        modeKnob_->setBounds(row1.removeFromLeft(knobWidth).reduced(4));
        rateModeKnob_->setBounds(row1.removeFromLeft(knobWidth).reduced(4));
        rateHzKnob_->setBounds(row1.reduced(4));

        auto row2 = content.removeFromTop(108);
        syncDivisionKnob_->setBounds(row2.removeFromLeft(knobWidth).reduced(4));
        octaveRangeKnob_->setBounds(row2.removeFromLeft(knobWidth).reduced(4));
        numStepsKnob_->setBounds(row2.reduced(4));

        auto row3 = content.removeFromTop(108);
        latchKnob_->setBounds(row3.removeFromLeft(knobWidth).reduced(4));

        content.removeFromTop(8);
        stepStrip_.setBounds(content.removeFromTop(72));
    }

    void ArpPanelOverlay::mouseDown(const juce::MouseEvent& event)
    {
        if (!drawer_.getBounds().contains(event.getPosition()))
            dismiss();
    }

    bool ArpPanelOverlay::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
