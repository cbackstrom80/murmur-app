#include "ObsidianLookAndFeel.h"

#include <cmath>

#include "DeckedKnobDraw.h"
#include "ObsidianDraw.h"
#include "ObsidianFonts.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"
#include "RadialGlowDraw.h"

namespace pw8::plugin::ui
{
    ObsidianLookAndFeel::ObsidianLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, palette::kBackgroundTop);
        setColour(juce::Label::textColourId, palette::kTextPrimary);
        setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::rotarySliderFillColourId, palette::kMurmurViolet);
        setColour(juce::ToggleButton::textColourId, palette::kTextSecondary);
        setColour(juce::TooltipWindow::backgroundColourId, palette::kPanelRaised);
        setColour(juce::TooltipWindow::textColourId, palette::kTextPrimary);

        setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        setColour(juce::TextButton::buttonOnColourId, palette::kAccentDim);
        setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);

        setColour(juce::TextEditor::backgroundColourId, palette::kPanel);
        setColour(juce::TextEditor::textColourId, palette::kTextPrimary);
        setColour(juce::TextEditor::outlineColourId, palette::kBorder);
        setColour(juce::TextEditor::focusedOutlineColourId, palette::kAccent);
        setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::ScrollBar::thumbColourId, palette::kBorderBright);

        setColour(juce::ComboBox::backgroundColourId, palette::kPanelRaised);
        setColour(juce::ComboBox::textColourId, palette::kTextPrimary);
        setColour(juce::ComboBox::outlineColourId, palette::kBorder);
        setColour(juce::ComboBox::arrowColourId, palette::kTextSecondary);
        setColour(juce::ComboBox::buttonColourId, palette::kPanel);

        setColour(juce::PopupMenu::backgroundColourId, palette::kPanelRaised);
        setColour(juce::PopupMenu::textColourId, palette::kTextPrimary);
        setColour(juce::PopupMenu::headerTextColourId, palette::kTextSecondary);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, palette::kAccentDim);
        setColour(juce::PopupMenu::highlightedTextColourId, palette::kTextPrimary);
    }

    void ObsidianLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& /*backgroundColour*/,
                                                    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        const auto bounds = button.getLocalBounds().toFloat();
        const float corner = juce::jmin(8.0f, bounds.getHeight() * 0.28f);
        const bool toggledOn = button.getToggleState();

        juce::Colour accent = palette::kAccent;
        if (auto* textButton = dynamic_cast<juce::TextButton*>(&button))
        {
            if (textButton->getToggleState())
                accent = textButton->findColour(juce::TextButton::buttonOnColourId);
            if (accent.isTransparent())
                accent = palette::kAccent;
        }

        draw::paintButtonFace(g, bounds, corner, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown, toggledOn,
                              toggledOn ? palette::kFigmaPillActive : accent);
    }

    juce::Font ObsidianLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
    {
        juce::ignoreUnused(button);
        return fonts::label(juce::jlimit(fonts::kLabelMinSize, 12.0f, static_cast<float>(buttonHeight) * 0.38f));
    }

    void ObsidianLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                        juce::TextEditor& /*editor*/)
    {
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        draw::fillRecessedRoundedRect(g, bounds.reduced(0.5f), 4.0f);
    }

    void ObsidianLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                                     juce::TextEditor& editor)
    {
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
                                .reduced(0.5f);
        if (editor.hasKeyboardFocus(true))
        {
            auto outline = draw::roundedRectPath(bounds, 4.0f);
            draw::strokeGlowPath(g, outline, 0.85f, 1.2f, true);
        }
        else
        {
            g.setColour(palette::kBorder);
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }
    }

    void ObsidianLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
    {
        if (dynamic_cast<juce::Slider*>(label.getParentComponent()) != nullptr)
        {
            const auto bounds = label.getLocalBounds().toFloat().reduced(0.5f);
            draw::fillRecessedRoundedRect(g, bounds, 3.0f);
            g.setColour(palette::kBorder);
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setColour(label.findColour(juce::Label::textColourId));
            g.setFont(label.getFont());
            g.drawText(label.getText(), label.getLocalBounds(), label.getJustificationType(), true);
            return;
        }

        LookAndFeel_V4::drawLabel(g, label);
    }

    int ObsidianLookAndFeel::getDefaultScrollbarWidth()
    {
        return 6;
    }

    void ObsidianLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& /*scrollbar*/, int x, int y, int width,
                                           int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                           bool isMouseOver, bool isMouseDown)
    {
        juce::ignoreUnused(isScrollbarVertical);

        if (thumbSize <= 0)
            return;

        auto thumb = isScrollbarVertical ? juce::Rectangle<float>(static_cast<float>(x),
                                                                   static_cast<float>(thumbStartPosition),
                                                                   static_cast<float>(width),
                                                                   static_cast<float>(thumbSize))
                                         : juce::Rectangle<float>(static_cast<float>(thumbStartPosition),
                                                                   static_cast<float>(y),
                                                                   static_cast<float>(thumbSize),
                                                                   static_cast<float>(height));

        thumb = thumb.reduced(isScrollbarVertical ? 1.0f : 2.0f, isScrollbarVertical ? 2.0f : 1.0f);
        const float corner = juce::jmin(thumb.getWidth(), thumb.getHeight()) * 0.5f;

        g.setColour(isMouseDown ? palette::kAccent : (isMouseOver ? palette::kAccentDim : palette::kBorderBright));
        g.fillRoundedRectangle(thumb, corner);
    }

    void ObsidianLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPosProportional, float rotaryStartAngle,
                                                float rotaryEndAngle, juce::Slider& slider)
    {
        if (width <= 0 || height <= 0)
            return;

        const auto ringRole = slider.getProperties()["knobRingRole"].toString();
        const bool outerOnly = ringRole == "outer";
        const bool innerRole = ringRole == "inner";
        const auto knobStyle = slider.getProperties()["knobStyle"].toString();
        const bool deckedStyle = knobStyle == "decked";
        const bool radialGlowStyle = knobStyle == "radialGlow";

        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                     static_cast<float>(width), static_cast<float>(height))
                                 .reduced(4.0f);
        const float maxDial =
            static_cast<float>(slider.getProperties().getWithDefault("maxDialDiameter", 88));
        const float diameter =
            juce::jmin(maxDial, juce::jmax(16.0f, juce::jmin(bounds.getWidth(), bounds.getHeight())));
        const auto knobBounds = bounds.withSizeKeepingCentre(diameter, diameter);
        const float radius = diameter * 0.5f;
        const auto centre = knobBounds.getCentre();
        const float proportional = rotary::normalisedProportional(sliderPosProportional, slider);
        const float angle = rotary::proportionalToAngle(proportional, rotaryStartAngle, rotaryEndAngle);
        const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

        if (radialGlowStyle)
        {
            const bool featuredKoin = slider.getProperties().getWithDefault("featuredKoin", false);
            const juce::String valueText = slider.getTextFromValue(slider.getValue());
            radialglow::drawSinglePerformanceKnob(g, knobBounds, proportional, rotaryStartAngle, rotaryEndAngle, accent,
                                                  featuredKoin, valueText, slider.isMouseOverOrDragging());
            return;
        }

        if (deckedStyle)
        {
            const auto deckedSize = decked::sizeFromProperty(slider.getProperties()["deckedSize"].toString());
            const bool active = slider.isMouseOverOrDragging();
            const bool featuredKoin = slider.getProperties().getWithDefault("featuredKoin", false);
            auto drawAccent = accent;
            if (featuredKoin)
            {
                drawAccent = accent.isTransparent() ? palette::kAccentWarm : accent.brighter(0.10f);
                if (drawAccent.isTransparent())
                    drawAccent = palette::kAccentWarm;
            }
            drawDeckedRotarySlider(g, knobBounds, proportional, rotaryStartAngle, rotaryEndAngle, drawAccent, deckedSize,
                                   outerOnly, innerRole, active, featuredKoin);
            return;
        }

        const bool drawSatellites = !outerOnly && diameter >= 48.0f;
        const bool drawSatelliteGlow = !outerOnly && diameter >= 58.0f;
        const bool drawOrbitHalo = !outerOnly && diameter >= 44.0f;
        const bool drawDropShadow = !outerOnly && diameter >= 44.0f;

        const float orbitRadius = radius * 0.90f;
        const float bodyRadius = radius * 0.62f;
        const float trackThickness = juce::jmax(2.0f, radius * (drawSatellites ? 0.068f : 0.058f));
        const float valueArcRadius = outerOnly ? orbitRadius : (innerRole ? bodyRadius * 0.92f : orbitRadius);
        const juce::Point<float> direction = rotary::unitDirectionAtAngle(angle);

        if (!outerOnly && !innerRole)
            radialglow::drawBackgroundHalo(g, centre, orbitRadius, accent.isTransparent() ? palette::kAccent : accent);

        if (outerOnly)
        {
            juce::Path track;
            track.addCentredArc(centre.x, centre.y, orbitRadius, orbitRadius, 0.0f, rotaryStartAngle, rotaryEndAngle,
                                true);
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.strokePath(track, juce::PathStrokeType(trackThickness * 0.85f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

            if (proportional > 0.001f)
            {
                juce::Path value;
                value.addCentredArc(centre.x, centre.y, orbitRadius, orbitRadius, 0.0f, rotaryStartAngle, angle, true);
                g.setColour(accent.withAlpha(0.22f));
                g.strokePath(value, juce::PathStrokeType(trackThickness * 2.85f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
                g.setColour(accent.withAlpha(0.94f));
                g.strokePath(value, juce::PathStrokeType(trackThickness * 0.78f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
            }

            const float pointerInner = orbitRadius * 0.82f;
            const float pointerOuter = orbitRadius * 0.98f;
            const auto pointerStart = centre + direction * pointerInner;
            const auto pointerEnd = centre + direction * pointerOuter;
            g.setColour(accent.withAlpha(0.25f));
            g.drawLine({pointerStart, pointerEnd}, juce::jmax(2.5f, radius * 0.05f));
            g.setColour(juce::Colour(0xffeee8ff).interpolatedWith(accent, 0.55f));
            g.drawLine({pointerStart, pointerEnd}, juce::jmax(1.4f, radius * 0.024f));
            return;
        }

        if (drawDropShadow)
        {
            g.setColour(palette::kShadow.withAlpha(0.35f));
            g.fillEllipse(centre.x - radius + 1.0f, centre.y - radius + 2.5f, radius * 2.0f, radius * 2.0f);
        }

        g.setColour(palette::kBackgroundBottom);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(palette::kBorder);
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f,
                      juce::jmax(1.0f, radius * 0.024f));

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, valueArcRadius, valueArcRadius, 0.0f, rotaryStartAngle,
                            rotaryEndAngle, true);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.strokePath(track, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        if (proportional > 0.001f)
        {
            juce::Path value;
            value.addCentredArc(centre.x, centre.y, valueArcRadius, valueArcRadius, 0.0f, rotaryStartAngle, angle,
                                true);
            g.setColour(accent.withAlpha(0.24f));
            g.strokePath(value, juce::PathStrokeType(trackThickness * 2.85f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.setColour(accent.withAlpha(0.94f));
            g.strokePath(value, juce::PathStrokeType(trackThickness * 0.78f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        if (drawOrbitHalo)
        {
            g.setColour(palette::kMurmurVioletDeep.withAlpha(drawSatelliteGlow ? 0.16f : 0.10f));
            g.drawEllipse(centre.x - orbitRadius, centre.y - orbitRadius, orbitRadius * 2.0f, orbitRadius * 2.0f,
                          juce::jmax(1.2f, radius * 0.04f));
        }

        if (drawSatellites)
        {
            const float satelliteAlpha = drawSatelliteGlow ? 0.72f : 0.38f;
            const float dotRadius = juce::jmax(1.5f, radius * (drawSatelliteGlow ? 0.035f : 0.028f));
            for (int i = 0; i < 8; ++i)
            {
                const auto satelliteAngle = -juce::MathConstants<float>::halfPi
                                          + (juce::MathConstants<float>::twoPi * static_cast<float>(i) / 8.0f);
                const auto point = centre + rotary::unitDirectionAtAngle(satelliteAngle) * orbitRadius;
                if (drawSatelliteGlow)
                    draw::fillGlowDot(g, point, dotRadius, palette::kMurmurViolet, satelliteAlpha);
                else
                {
                    g.setColour(palette::kMurmurViolet.withAlpha(satelliteAlpha));
                    g.fillEllipse(point.x - dotRadius, point.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
                }
            }
        }

        juce::ColourGradient bodyGradient(juce::Colour(0xff303844), centre.x - bodyRadius * 0.4f,
                                          centre.y - bodyRadius * 0.5f, juce::Colour(0xff070a0e),
                                          centre.x + bodyRadius * 0.5f, centre.y + bodyRadius * 0.6f, false);
        bodyGradient.addColour(0.48, juce::Colour(0xff121820));
        g.setGradientFill(bodyGradient);
        g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        juce::ColourGradient domeHighlight(juce::Colour(0xff242a33).withAlpha(0.55f), centre.x - bodyRadius * 0.25f,
                                           centre.y - bodyRadius * 0.35f, juce::Colours::transparentBlack,
                                           centre.x + bodyRadius * 0.2f, centre.y + bodyRadius * 0.25f, true);
        g.setGradientFill(domeHighlight);
        g.fillEllipse(centre.x - bodyRadius * 0.88f, centre.y - bodyRadius * 0.88f, bodyRadius * 1.76f,
                      bodyRadius * 1.76f);

        g.setColour(juce::Colour(0xff35404c));
        g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f,
                      juce::jmax(1.0f, radius * 0.024f));

        const float pointerInner = bodyRadius * 0.36f;
        const float pointerOuter = bodyRadius * 0.78f;
        const auto pointerStart = centre + direction * pointerInner;
        const auto pointerEnd = centre + direction * pointerOuter;

        if (drawOrbitHalo)
        {
            g.setColour(accent.withAlpha(0.20f));
            g.drawLine({pointerStart, pointerEnd}, juce::jmax(3.0f, radius * 0.055f));
        }
        g.setColour(juce::Colour(0xffeee8ff).interpolatedWith(accent, 0.35f));
        g.drawLine({pointerStart, pointerEnd}, juce::jmax(1.2f, radius * 0.021f));
    }

    void ObsidianLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                bool /*shouldDrawButtonAsHighlighted*/,
                                                bool /*shouldDrawButtonAsDown*/)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const float h = bounds.getHeight();
        const auto pillBounds = bounds.removeFromLeft(juce::jmin(bounds.getWidth(), h * 1.9f));
        const bool on = button.getToggleState();

        if (on)
        {
            draw::fillRecessedRoundedRect(g, pillBounds, h * 0.5f);
            g.setColour(palette::kAccent.withAlpha(0.18f));
            g.fillRoundedRectangle(pillBounds, h * 0.5f);
            draw::strokeGlowPath(g, draw::roundedRectPath(pillBounds, h * 0.5f), 0.85f, 1.2f, true);
        }
        else
        {
            draw::fillRecessedRoundedRect(g, pillBounds, h * 0.5f);
            g.setColour(palette::kBorder);
            g.drawRoundedRectangle(pillBounds, h * 0.5f, 1.0f);
        }

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
        return fonts::label(fonts::kBodyLabelSize);
    }

    juce::Font ObsidianLookAndFeel::getPopupMenuFont()
    {
        return fonts::label(fonts::kBodyLabelSize);
    }

    juce::Font ObsidianLookAndFeel::getComboBoxFont(juce::ComboBox& box)
    {
        juce::ignoreUnused(box);
        return fonts::label(fonts::kBodyLabelSize);
    }

} // namespace pw8::plugin::ui
