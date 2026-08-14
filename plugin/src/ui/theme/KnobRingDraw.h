#pragma once

#include <cmath>

#include <juce_graphics/juce_graphics.h>

#include "ObsidianDraw.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"

namespace pw8::plugin::ui::knobrings
{
    /// Shared layout for value arc (LAF) vs mod-route overlays (GlowKnob).
    struct Layout
    {
        juce::Point<float> centre{};
        float dialRadius = 0.0f;
        float valueArcRadius = 0.0f;
        float modRingRadius = 0.0f;
        float modStrokeWidth = 3.8f;
    };

    [[nodiscard]] inline Layout computeLayout(juce::Rectangle<float> sliderBounds, float maxDialDiameter,
                                              bool deckedStyle) noexcept
    {
        const float diameter =
            juce::jmin(maxDialDiameter, juce::jmax(16.0f, juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight())));
        const auto knobBounds = sliderBounds.withSizeKeepingCentre(diameter, diameter);
        const float dialRadius = diameter * 0.5f;
        const auto centre = knobBounds.getCentre();

        // Match decked::computeGeometry (medium) — value arc sits on middle deck.
        const float valueArcRadius = deckedStyle ? dialRadius * 0.80f * 0.94f : dialRadius * 0.90f;
        const float outerDeckRadius = deckedStyle ? dialRadius * 0.95f : dialRadius * 0.98f;
        const float modGap = juce::jmax(4.5f, dialRadius * 0.085f);
        const float modRingRadius = outerDeckRadius + modGap;
        const float modStrokeWidth = juce::jmax(3.8f, dialRadius * 0.078f);

        return {centre, dialRadius, valueArcRadius, modRingRadius, modStrokeWidth};
    }

    inline void strokeGlowArc(juce::Graphics& g, const juce::Path& arc, juce::Colour colour, float strokeWidth)
    {
        g.setColour(colour.withAlpha(0.24f));
        g.strokePath(arc, juce::PathStrokeType(strokeWidth * 1.85f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        g.setColour(colour.withAlpha(0.94f));
        g.strokePath(arc, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    inline void strokeModRouteArc(juce::Graphics& g, const Layout& layout, float amountNormalized,
                                  juce::Colour sourceColour, float strokeScale = 1.0f)
    {
        if (sourceColour.isTransparent() || amountNormalized <= 0.001f)
            return;

        const float endAngle =
            rotary::kStartAngle + rotary::kSweep * juce::jlimit(0.0f, 1.0f, amountNormalized);
        juce::Path arc;
        arc.addCentredArc(layout.centre.x, layout.centre.y, layout.modRingRadius, layout.modRingRadius, 0.0f,
                          rotary::kStartAngle, endAngle, true);
        strokeGlowArc(g, arc, sourceColour, layout.modStrokeWidth * strokeScale);
    }

    inline void strokeModRouteTrack(juce::Graphics& g, const Layout& layout, juce::Colour trackColour,
                                    float strokeScale = 1.0f)
    {
        juce::Path track;
        track.addCentredArc(layout.centre.x, layout.centre.y, layout.modRingRadius, layout.modRingRadius, 0.0f,
                            rotary::kStartAngle, rotary::kEndAngle, true);
        g.setColour(trackColour);
        g.strokePath(track, juce::PathStrokeType(layout.modStrokeWidth * strokeScale * 0.45f,
                                                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    inline void drawLiveModGhost(juce::Graphics& g, const Layout& layout, float liveNormalized,
                                 juce::Colour accent)
    {
        if (liveNormalized < 0.0f)
            return;

        const float ghostAngle =
            rotary::kStartAngle + rotary::kSweep * juce::jlimit(0.0f, 1.0f, liveNormalized);
        const auto direction = rotary::unitDirectionAtAngle(ghostAngle);
        const auto point = layout.centre + direction * layout.modRingRadius;
        const float dotRadius = juce::jmax(2.8f, layout.dialRadius * 0.048f);
        draw::fillGlowDot(g, point, dotRadius, accent, 0.88f, 5);
    }

    inline void drawDragHoverRing(juce::Graphics& g, const Layout& layout)
    {
        g.setColour(palette::kMurmurViolet.withAlpha(0.65f));
        g.drawEllipse(layout.centre.x - layout.modRingRadius - 2.0f, layout.centre.y - layout.modRingRadius - 2.0f,
                      (layout.modRingRadius + 2.0f) * 2.0f, (layout.modRingRadius + 2.0f) * 2.0f,
                      juce::jmax(2.0f, layout.modStrokeWidth * 0.55f));
    }

} // namespace pw8::plugin::ui::knobrings
