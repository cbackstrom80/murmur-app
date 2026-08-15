#pragma once

#include <juce_graphics/juce_graphics.h>

#include "ObsidianPalette.h"

namespace pw8::quasar::ui::draw
{
    /// Layered alpha circles — scalable glow without bitmap blur (from Murmur UI kit).
    inline void fillGlowDot(juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour,
                            float intensity, int glowLayers = 6)
    {
        for (int i = glowLayers; i >= 1; --i)
        {
            const auto r = radius * (1.0f + 0.5f * static_cast<float>(i));
            g.setColour(colour.withAlpha(intensity * 0.025f * static_cast<float>(glowLayers + 1 - i)));
            g.fillEllipse(centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        }

        g.setColour(colour.withAlpha(juce::jlimit(0.0f, 1.0f, intensity)));
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colours::white.withAlpha(0.70f * intensity));
        const auto core = radius * 0.42f;
        g.fillEllipse(centre.x - core, centre.y - core, core * 2.0f, core * 2.0f);
    }

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

    /// Subtle dark pill + soft shadow so labels stay readable over spectrum/glow backgrounds.
    inline void drawLegibleText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> bounds,
                                juce::Justification justification, juce::Colour textColour, const juce::Font& font,
                                bool withBacking = true)
    {
        if (withBacking)
        {
            auto pill = bounds.expanded(5.0f, 2.5f);
            g.setColour(palette::kBackgroundBottom.withAlpha(0.78f));
            g.fillRoundedRectangle(pill, 4.0f);
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(pill, 4.0f, 1.0f);
        }

        g.setFont(font);
        g.setColour(textColour.withAlpha(0.28f));
        g.drawText(text, bounds.translated(0.0f, 1.0f), justification, true);
        g.setColour(textColour);
        g.drawText(text, bounds, justification, true);
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

} // namespace pw8::quasar::ui::draw
