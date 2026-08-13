#include "ObsidianLookAndFeel.h"

#include "ObsidianDraw.h"
#include "ObsidianFonts.h"
#include "ObsidianPalette.h"

namespace pw8::plugin::ui
{
    ObsidianLookAndFeel::ObsidianLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, palette::kBackgroundTop);
        setColour(juce::Label::textColourId, palette::kTextPrimary);
        setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::rotarySliderFillColourId, palette::kAccent);
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
                              accent);
    }

    juce::Font ObsidianLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
    {
        juce::ignoreUnused(button);
        return fonts::label(juce::jlimit(9.5f, 12.0f, static_cast<float>(buttonHeight) * 0.38f));
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
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

        const float trackThickness = juce::jmax(2.0f, radius * 0.14f);
        const float trackRadius = radius - trackThickness * 0.5f - 1.0f;
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, rotaryEndAngle,
                             true);
        g.setColour(palette::kBorder);
        g.strokePath(track, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        if (sliderPosProportional > 0.001f)
        {
            juce::Path value;
            value.addCentredArc(centre.x, centre.y, trackRadius, trackRadius, 0.0f, rotaryStartAngle, angle, true);
            draw::strokeGlowPath(g, value, 1.0f, trackThickness, true);
        }

        const float bodyRadius = juce::jmax(2.0f, trackRadius - trackThickness * 1.6f);
        juce::ColourGradient bodyGradient(palette::kPanelRaised, centre.x, centre.y - bodyRadius,
                                           palette::kPanel, centre.x, centre.y + bodyRadius, false);
        g.setGradientFill(bodyGradient);
        g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);
        g.setColour(palette::kBorderBright);
        g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

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
        return fonts::label(12.0f);
    }

} // namespace pw8::plugin::ui
