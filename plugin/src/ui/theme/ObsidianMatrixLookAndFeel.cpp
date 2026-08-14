#include "ObsidianMatrixLookAndFeel.h"

#include "ObsidianDraw.h"
#include "ObsidianFonts.h"
#include "ObsidianPalette.h"

namespace pw8::plugin::ui
{
    ObsidianMatrixLookAndFeel::ObsidianMatrixLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, palette::kTextPrimary);
        setColour(juce::Slider::trackColourId, palette::kPanel.withAlpha(0.65f));
        setColour(juce::Slider::textBoxTextColourId, palette::kTextSecondary);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

        setColour(juce::ComboBox::backgroundColourId, palette::kPanel.withAlpha(0.55f));
        setColour(juce::ComboBox::outlineColourId, palette::kBorder.withAlpha(0.85f));
        setColour(juce::ComboBox::textColourId, palette::kTextPrimary.withAlpha(0.85f));
        setColour(juce::ComboBox::arrowColourId, palette::kTextSecondary);
        setColour(juce::ComboBox::buttonColourId, juce::Colours::transparentBlack);

        setColour(juce::ToggleButton::tickColourId, palette::kAccent);
        setColour(juce::ToggleButton::tickDisabledColourId, palette::kTextDim);

        setColour(juce::PopupMenu::backgroundColourId, palette::kPanelRaised);
        setColour(juce::PopupMenu::textColourId, palette::kTextPrimary);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, palette::kAccentDim);
        setColour(juce::PopupMenu::highlightedTextColourId, palette::kTextPrimary);
    }

    void ObsidianMatrixLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                                       float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                                       juce::Slider::SliderStyle, juce::Slider& slider)
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(0.0f, 6.0f);
        if (bounds.isEmpty())
            return;

        const float corner = bounds.getHeight() * 0.5f;

        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(bounds, corner);
        g.setColour(palette::kTopHighlight.withAlpha(0.08f));
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        const float minVal = static_cast<float>(slider.getMinimum());
        const float maxVal = static_cast<float>(slider.getMaximum());
        const float zeroNorm =
            (minVal < 0.0f && maxVal > 0.0f) ? slider.valueToProportionOfLength(0.0) : 0.5f;
        const float midX = bounds.getX() + bounds.getWidth() * zeroNorm;

        auto fillRect = juce::Rectangle<float>(midX, bounds.getY(), sliderPos - midX, bounds.getHeight());
        if (sliderPos < midX)
        {
            fillRect.setLeft(sliderPos);
            fillRect.setRight(midX);
        }

        if (fillRect.getWidth() > 0.5f)
        {
            const auto accent = palette::kAccent;
            juce::ColourGradient fillGradient(accent.withAlpha(0.55f), midX, bounds.getY(),
                                            accent.withAlpha(0.95f), sliderPos, bounds.getY(), false);
            g.setGradientFill(fillGradient);
            g.fillRoundedRectangle(fillRect, fillRect.getHeight() * 0.5f);
        }

        const float thumbW = 8.0f;
        const float thumbH = bounds.getHeight() + 4.0f;
        auto thumbBounds =
            juce::Rectangle<float>(sliderPos - thumbW * 0.5f, bounds.getY() - 2.0f, thumbW, thumbH);
        g.setColour(palette::kTextPrimary);
        g.fillRoundedRectangle(thumbBounds, 3.0f);
        g.setColour(palette::kAccent.withAlpha(0.35f));
        g.drawRoundedRectangle(thumbBounds.expanded(0.5f), 3.0f, 1.0f);
    }

    void ObsidianMatrixLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                                  int buttonX, int buttonY, int buttonW, int buttonH,
                                                  juce::ComboBox& box)
    {
        const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height))
                                .reduced(0.5f);
        const float corner = juce::jmin(5.0f, bounds.getHeight() * 0.28f);

        draw::fillRecessedRoundedRect(g, bounds, corner);
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, corner, 1.0f);

        if (box.hasKeyboardFocus(false))
        {
            draw::strokeGlowPath(g, draw::roundedRectPath(bounds, corner), 0.75f, 1.0f, true);
        }

        const auto arrowBounds =
            juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                   static_cast<float>(buttonW), static_cast<float>(buttonH))
                .reduced(static_cast<float>(buttonW) * 0.28f, static_cast<float>(buttonH) * 0.32f);

        juce::Path arrow;
        arrow.startNewSubPath(arrowBounds.getX(), arrowBounds.getY());
        arrow.lineTo(arrowBounds.getCentreX(), arrowBounds.getBottom());
        arrow.lineTo(arrowBounds.getRight(), arrowBounds.getY());
        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.strokePath(arrow, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void ObsidianMatrixLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                      bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const float corner = juce::jmin(5.0f, bounds.getHeight() * 0.28f);
        const bool on = button.getToggleState();
        const auto accent = button.findColour(juce::ToggleButton::tickColourId);

        if (on)
        {
            draw::fillRecessedRoundedRect(g, bounds, corner);
            g.setColour(accent.withAlpha(0.16f));
            g.fillRoundedRectangle(bounds, corner);
            draw::strokeGlowPath(g, draw::roundedRectPath(bounds, corner), 0.85f, 1.1f, true);
        }
        else
        {
            draw::fillRecessedRoundedRect(g, bounds, corner);
            const auto border = shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown ? palette::kBorderBright
                                                                                        : palette::kBorder;
            g.setColour(border);
            g.drawRoundedRectangle(bounds, corner, 1.0f);
        }

        if (on)
        {
            g.setColour(accent);
            g.fillEllipse(bounds.reduced(bounds.getWidth() * 0.28f, bounds.getHeight() * 0.28f));
        }
    }

    juce::Font ObsidianMatrixLookAndFeel::getComboBoxFont(juce::ComboBox& box)
    {
        juce::ignoreUnused(box);
        return fonts::label(10.5f);
    }

} // namespace pw8::plugin::ui
