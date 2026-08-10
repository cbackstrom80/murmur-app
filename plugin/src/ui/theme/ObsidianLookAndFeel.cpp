#include "ObsidianLookAndFeel.h"

#include "ObsidianFonts.h"
#include "ObsidianPalette.h"

namespace pw8::plugin::ui
{
    ObsidianLookAndFeel::ObsidianLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, palette::kBackgroundTop);
        setColour(juce::Label::textColourId, palette::kTextPrimary);
        setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        // The default every knob's value arc/pointer paints in unless a specific
        // GlowKnob asks for the warm variant instead -- see "duotone" in
        // ObsidianPalette.h. Repurposing JUCE's own rotarySliderFillColourId
        // rather than inventing a parallel mechanism keeps per-knob accent choice
        // a one-line `slider.setColour(...)` at the call site.
        setColour(juce::Slider::rotarySliderFillColourId, palette::kAccent);
        setColour(juce::ToggleButton::textColourId, palette::kTextSecondary);
        setColour(juce::TooltipWindow::backgroundColourId, palette::kPanelRaised);
        setColour(juce::TooltipWindow::textColourId, palette::kTextPrimary);
    }

    void ObsidianLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPosProportional, float rotaryStartAngle,
                                                float rotaryEndAngle, juce::Slider& slider)
    {
        // Defensive floor, not just a layout-correctness assumption: every radius
        // below is derived from `diameter`, so an under-sized allotment (a future
        // layout bug, a host resizing the editor unexpectedly, ...) must not be
        // allowed to walk the chain negative -- that produced a real, caught bug
        // during development (a starved knob's derived body radius went negative,
        // which juce::Graphics's own debug assertions flagged as a malformed
        // fillEllipse call). 16px is comfortably above where any term below would
        // cross zero.
        if (width <= 0 || height <= 0)
            return;

        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                     static_cast<float>(width), static_cast<float>(height))
                                 .reduced(4.0f);
        const float diameter = juce::jmax(16.0f, juce::jmin(bounds.getWidth(), bounds.getHeight()));
        const auto knobBounds = bounds.withSizeKeepingCentre(diameter, diameter);
        const float radius = diameter * 0.5f;
        const auto centre = knobBounds.getCentre();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

        // Background track: a full arc from start to end angle, quiet and recessed.
        const float trackThickness = juce::jmax(2.0f, radius * 0.14f);
        const float trackRadius = radius - trackThickness * 0.5f - 1.0f;
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, rotaryEndAngle,
                             true);
        g.setColour(palette::kBorder);
        g.strokePath(track, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Value arc: the accent, with a soft under-glow (a wider, translucent stroke
        // painted first) so it reads as *lit* rather than just colored -- this is
        // the one place in the whole skin the accent is allowed to be prominent.
        if (sliderPosProportional > 0.001f)
        {
            juce::Path value;
            value.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, angle, true);

            g.setColour(accent.withAlpha(0.32f));
            g.strokePath(value, juce::PathStrokeType(trackThickness * 3.2f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            g.setColour(accent.withAlpha(0.5f));
            g.strokePath(value, juce::PathStrokeType(trackThickness * 1.8f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

            g.setColour(accent);
            g.strokePath(value, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        // Knob body: a flat cylinder, not skeuomorphic chrome -- a subtle top-lit
        // radial gradient is the only concession to dimensionality.
        const float bodyRadius = juce::jmax(2.0f, trackRadius - trackThickness * 1.6f);
        juce::ColourGradient bodyGradient(palette::kPanelRaised, centre.x, centre.y - bodyRadius,
                                           palette::kPanel, centre.x, centre.y + bodyRadius, false);
        g.setGradientFill(bodyGradient);
        g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
        g.setColour(palette::kBorderBright);
        g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

        // Indicator: a single thin line from just outside the body centre to near
        // its rim, at the current angle -- the precise, "engineered" read.
        juce::Path pointer;
        const float pointerInner = bodyRadius * 0.25f;
        const float pointerOuter = bodyRadius * 0.92f;
        pointer.startNewSubPath(centre.x, centre.y - pointerInner);
        pointer.lineTo(centre.x, centre.y - pointerOuter);
        pointer.applyTransform(juce::AffineTransform::rotation(angle, centre.x, centre.y));
        g.setColour(accent);
        g.strokePath(pointer, juce::PathStrokeType(juce::jmax(1.5f, bodyRadius * 0.09f),
                                                     juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void ObsidianLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                bool /*shouldDrawButtonAsHighlighted*/,
                                                bool /*shouldDrawButtonAsDown*/)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const float h = bounds.getHeight();
        // Left-aligned, not centred: the pill is only ever part of the button's
        // width (the rest is the text label), so centring it here would place it
        // on top of where the label needs to start. `removeFromLeft` also shrinks
        // `bounds` down to the remaining label area, used for the text below.
        const auto pillBounds = bounds.removeFromLeft(juce::jmin(bounds.getWidth(), h * 1.9f));
        const bool on = button.getToggleState();

        g.setColour(on ? palette::kAccentDim : palette::kPanel);
        g.fillRoundedRectangle(pillBounds, h * 0.5f);
        g.setColour(palette::kBorderBright);
        g.drawRoundedRectangle(pillBounds, h * 0.5f, 1.0f);

        const float knobDiameter = h * 0.72f;
        const float travel = pillBounds.getWidth() - knobDiameter - h * 0.14f;
        const float knobX = pillBounds.getX() + h * 0.07f + (on ? travel : 0.0f);
        g.setColour(on ? palette::kAccent : palette::kTextDim);
        g.fillEllipse(knobX, pillBounds.getCentreY() - knobDiameter * 0.5f, knobDiameter, knobDiameter);

        if (button.getButtonText().isNotEmpty())
        {
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(12.0f));
            g.drawText(button.getButtonText(), bounds.withTrimmedLeft(6.0f), juce::Justification::centredLeft, true);
        }
    }

    juce::Font ObsidianLookAndFeel::getLabelFont(juce::Label& label)
    {
        juce::ignoreUnused(label);
        return fonts::label(12.0f);
    }

} // namespace pw8::plugin::ui
