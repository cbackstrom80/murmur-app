#include "LabLauncherChip.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    LabLauncherChip::LabLauncherChip()
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void LabLauncherChip::setLabel(const juce::String& label)
    {
        label_ = label;
        repaint();
    }

    void LabLauncherChip::setAccentColour(juce::Colour colour)
    {
        accent_ = colour;
        repaint();
    }

    void LabLauncherChip::setHighlighted(bool highlighted)
    {
        highlighted_ = highlighted;
        repaint();
    }

    void LabLauncherChip::setIcon(LabLauncherIcon icon)
    {
        icon_ = icon;
        repaint();
    }

    juce::Rectangle<int> LabLauncherChip::getPreferredSize() noexcept
    {
        return {0, 0, layout::kLabChipWidth, layout::kLabChipHeight};
    }

    void LabLauncherChip::drawMiniIcon(juce::Graphics& g, juce::Rectangle<float> area) const
    {
        if (icon_ == LabLauncherIcon::Vocoder)
        {
            const float w = area.getWidth() / 4.0f;
            const float h = area.getHeight();
            g.fillRect(area.getX(), area.getY() + h * 0.15f, w * 0.55f, h * 0.85f);
            g.fillRect(area.getX() + w, area.getY(), w * 0.55f, h);
            g.fillRect(area.getX() + w * 2.0f, area.getY() + h * 0.45f, w * 0.55f, h * 0.55f);
        }
        else if (icon_ == LabLauncherIcon::Lfo)
        {
            juce::Path wave;
            const float x0 = area.getX();
            const float y0 = area.getCentreY();
            const float w = area.getWidth();
            wave.startNewSubPath(x0, y0);
            wave.quadraticTo(x0 + w * 0.25f, area.getY(), x0 + w * 0.5f, y0);
            wave.quadraticTo(x0 + w * 0.75f, area.getBottom(), x0 + w, y0);
            g.strokePath(wave, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else if (icon_ == LabLauncherIcon::Mod)
        {
            g.drawLine(area.getX(), area.getBottom(), area.getRight(), area.getY(), 1.2f);
            g.fillEllipse(area.getCentreX() - 1.5f, area.getCentreY() - 1.5f, 3.0f, 3.0f);
        }
    }

    void LabLauncherChip::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        const float radius = 4.0f;

        if (highlighted_)
        {
            g.setColour(accent_.withAlpha(0.13f));
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(accent_);
            g.drawRoundedRectangle(bounds, radius, 1.0f);
        }
        else
        {
            g.setColour(palette::kBackgroundTop);
            g.fillRoundedRectangle(bounds, radius);
            g.setColour(hovered_ ? palette::kBorderBright : palette::kBorder);
            g.drawRoundedRectangle(bounds, radius, 0.9f);
        }

        auto content = bounds.reduced(6.0f, 3.0f);
        const float dotR = 2.0f;
        g.setColour(accent_.withAlpha(highlighted_ ? 1.0f : 0.65f));
        g.fillEllipse(content.getX(), content.getCentreY() - dotR, dotR * 2.0f, dotR * 2.0f);
        content.removeFromLeft(8.0f);

        g.setFont(fonts::label(8.0f));
        g.setColour(highlighted_ ? palette::kTextPrimary : palette::kTextDim);
        auto textArea = content;
        if (icon_ != LabLauncherIcon::None)
            textArea.removeFromRight(12.0f);
        g.drawText(label_, textArea, juce::Justification::centredLeft);

        if (icon_ != LabLauncherIcon::None)
        {
            g.setColour(highlighted_ ? accent_ : palette::kTextSecondary);
            drawMiniIcon(g, content.removeFromRight(10.0f));
        }
    }

    void LabLauncherChip::resized() {}

    void LabLauncherChip::mouseUp(const juce::MouseEvent& event)
    {
        if (event.mouseWasClicked() && onClick)
            onClick();
    }

    void LabLauncherChip::mouseEnter(const juce::MouseEvent&) { hovered_ = true; repaint(); }

    void LabLauncherChip::mouseExit(const juce::MouseEvent&) { hovered_ = false; repaint(); }

} // namespace pw8::plugin::ui
