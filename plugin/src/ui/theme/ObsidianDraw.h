#pragma once

#include <juce_graphics/juce_graphics.h>

#include "ObsidianPalette.h"

namespace pw8::plugin::ui::draw
{
    /// Dual-pass accent stroke — same technique as wavetable mesh lines and knob value arcs.
    inline void strokeGlowPath(juce::Graphics& g, const juce::Path& path, float alpha, float strokeWidth, bool live)
    {
        g.setColour(palette::kAccent.withAlpha(alpha * 0.25f));
        g.strokePath(path, juce::PathStrokeType(strokeWidth * 2.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        g.setColour((live ? palette::kAccent : palette::kBorderBright).withAlpha(alpha));
        g.strokePath(path, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    inline void fillRecessedRoundedRect(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerSize)
    {
        juce::ColourGradient gradient(palette::kPanelRaised, bounds.getCentreX(), bounds.getY(),
                                      palette::kPanel, bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(palette::kTopHighlight.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1.0f), bounds.getX() + cornerSize,
                             bounds.getRight() - cornerSize);
    }

    inline juce::Path roundedRectPath(juce::Rectangle<float> bounds, float cornerSize)
    {
        juce::Path path;
        path.addRoundedRectangle(bounds, cornerSize);
        return path;
    }

    /// Shared chrome for TextButtons, tabs, and chips — recessed by default, accent glow when toggled on.
    inline void paintButtonFace(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerSize, bool highlighted,
                                bool down, bool toggledOn, juce::Colour accent)
    {
        bounds = bounds.reduced(0.5f);
        if (bounds.isEmpty())
            return;

        if (down)
        {
            g.setColour(palette::kPanel);
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        else
        {
            fillRecessedRoundedRect(g, bounds, cornerSize);
        }

        if (toggledOn)
        {
            auto outline = roundedRectPath(bounds, cornerSize);
            strokeGlowPath(g, outline, 0.95f, 1.4f, true);
            g.setColour(accent.withAlpha(0.14f));
            g.fillRoundedRectangle(bounds, cornerSize);
        }
        else
        {
            g.setColour(highlighted ? palette::kBorderBright : palette::kBorder);
            g.drawRoundedRectangle(bounds, cornerSize, highlighted ? 1.2f : 1.0f);
        }
    }

} // namespace pw8::plugin::ui::draw
