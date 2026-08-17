#include "ModRoutingOverlay.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../PlayModeLayout.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::Rectangle<int> centeredAspectPanel(juce::Rectangle<int> area, float fillRatio, double aspectRatio)
        {
            const int maxW = static_cast<int>(static_cast<float>(area.getWidth()) * fillRatio);
            const int maxH = static_cast<int>(static_cast<float>(area.getHeight()) * fillRatio);

            int panelW = maxW;
            int panelH = static_cast<int>(static_cast<double>(panelW) / aspectRatio);
            if (panelH > maxH)
            {
                panelH = maxH;
                panelW = static_cast<int>(static_cast<double>(panelH) * aspectRatio);
            }

            return area.withSizeKeepingCentre(panelW, panelH);
        }
    } // namespace

    ModRoutingOverlay::ModRoutingOverlay(PatchworkEightProcessor& processor,
                                         ModAssignmentController& assignmentController)
        : modSourceStrip_(processor, assignmentController)
    {
        setVisible(false);
        addAndMakeVisible(panel_);
        panel_.addAndMakeVisible(badgeLabel_);
        panel_.addAndMakeVisible(titleLabel_);
        panel_.addAndMakeVisible(subtitleLabel_);
        panel_.addAndMakeVisible(closeButton_);
        panel_.addAndMakeVisible(modSourceStrip_);

        badgeLabel_.setText("UX-09", juce::dontSendNotification);
        badgeLabel_.setFont(fonts::label(9.0f));
        badgeLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        badgeLabel_.setJustificationType(juce::Justification::centred);

        titleLabel_.setText("MOD MATRIX", juce::dontSendNotification);
        titleLabel_.setFont(fonts::title(16.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        titleLabel_.setJustificationType(juce::Justification::centredLeft);

        subtitleLabel_.setText("Connect a source (LFO, envelope, velocity) to a target (filter cutoff/resonance). "
                               "Pick source, then destination. Esc to close.",
                               juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::value(fonts::kBodyLabelSize));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        subtitleLabel_.setJustificationType(juce::Justification::centredLeft);

        closeButton_.onClick = [this] { dismiss(); };
        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
    }

    void ModRoutingOverlay::showOverlay()
    {
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        panel_.setVisible(true);
        resized();
        grabKeyboardFocus();
        toFront(true);
    }

    void ModRoutingOverlay::dismiss()
    {
        setVisible(false);
        setInterceptsMouseClicks(false, true);
        if (onClosed)
            onClosed();
    }

    void ModRoutingOverlay::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        closeButton_.setButtonText(embedded ? "← DESIGN" : "← PLAY BOARD");
        resized();
    }

    void ModRoutingOverlay::setRoutingContext(FilterPanelScope scope, int engineIndex)
    {
        modSourceStrip_.setRoutingContext(scope, engineIndex);
    }

    void ModRoutingOverlay::repaintModAssignmentState()
    {
        modSourceStrip_.repaintModAssignmentState();
    }

    void ModRoutingOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop.withAlpha(0.72f));

        auto panelBounds = panel_.getBounds().toFloat();
        draw::fillRecessedRoundedRect(g, panelBounds, 8.0f);
        draw::strokeGlowPath(g, draw::roundedRectPath(panelBounds, 8.0f), 0.55f, 1.2f, false);

        auto badge = badgeLabel_.getBounds().toFloat().expanded(4.0f, 2.0f);
        g.setColour(palette::kAccent.withAlpha(0.13f));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(palette::kAccent);
        g.drawRoundedRectangle(badge, 4.0f, 1.0f);
    }

    void ModRoutingOverlay::resized()
    {
        panel_.setBounds(centeredAspectPanel(getLocalBounds(), layout::kOverlayFillRatio, layout::kAspectRatio));
        auto bounds = panel_.getLocalBounds().reduced(14);

        const int headerHeight = embeddedInDesignMode_ ? layout::kDesignLabPanelHeaderHeight : 36;
        auto header = bounds.removeFromTop(headerHeight);
        closeButton_.setBounds(header.removeFromRight(120).reduced(2));
        badgeLabel_.setBounds(header.removeFromLeft(40).withSizeKeepingCentre(36, 18));
        header.removeFromLeft(8);
        titleLabel_.setBounds(header);

        bounds.removeFromTop(4);
        subtitleLabel_.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(8);
        modSourceStrip_.setBounds(bounds);
    }

    void ModRoutingOverlay::mouseDown(const juce::MouseEvent& event)
    {
        if (!panel_.getBounds().contains(event.getPosition()))
            dismiss();
    }

    bool ModRoutingOverlay::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
