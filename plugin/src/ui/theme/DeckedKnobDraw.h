#pragma once

#include <cmath>

#include <juce_graphics/juce_graphics.h>

#include "ObsidianDraw.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"

namespace pw8::plugin::ui::decked
{
    enum class Size
    {
        Small,
        Medium,
        Large,
    };

    [[nodiscard]] inline Size sizeFromProperty(const juce::String& value) noexcept
    {
        if (value == "large")
            return Size::Large;
        if (value == "small")
            return Size::Small;
        return Size::Medium;
    }

    [[nodiscard]] inline juce::String sizeToProperty(Size size)
    {
        switch (size)
        {
            case Size::Large: return "large";
            case Size::Small: return "small";
            default: return "medium";
        }
    }

    struct Geometry
    {
        float radius = 0.0f;
        float outerDeckRadius = 0.0f;
        float middleDeckRadius = 0.0f;
        float innerCapRadius = 0.0f;
        float valueArcRadius = 0.0f;
        float trackThickness = 1.5f;
        bool drawLedRing = false;
        bool drawDropShadow = false;
        bool drawRimGlow = false;
    };

    [[nodiscard]] inline Geometry computeGeometry(float diameter, Size size)
    {
        Geometry geo;
        geo.radius = diameter * 0.5f;

        switch (size)
        {
            case Size::Large:
                geo.outerDeckRadius = geo.radius * 0.96f;
                geo.middleDeckRadius = geo.radius * 0.82f;
                geo.innerCapRadius = geo.radius * 0.56f;
                geo.trackThickness = juce::jmax(2.0f, geo.radius * 0.045f);
                geo.drawLedRing = true;
                geo.drawDropShadow = true;
                geo.drawRimGlow = true;
                break;
            case Size::Small:
                geo.outerDeckRadius = geo.radius * 0.94f;
                geo.middleDeckRadius = geo.radius * 0.78f;
                geo.innerCapRadius = geo.radius * 0.52f;
                geo.trackThickness = juce::jmax(1.2f, geo.radius * 0.05f);
                break;
            default:
                geo.outerDeckRadius = geo.radius * 0.95f;
                geo.middleDeckRadius = geo.radius * 0.80f;
                geo.innerCapRadius = geo.radius * 0.55f;
                geo.trackThickness = juce::jmax(1.5f, geo.radius * 0.048f);
                geo.drawDropShadow = diameter >= 44.0f;
                geo.drawRimGlow = diameter >= 58.0f;
                break;
        }

        geo.valueArcRadius = geo.middleDeckRadius * 0.94f;
        return geo;
    }

    inline void strokeDeckRing(juce::Graphics& g, juce::Point<float> centre, float radius, float thickness,
                               juce::Colour rimColour, juce::Colour highlightColour, bool rimLit)
    {
        g.setColour(palette::kShadow.withAlpha(0.28f));
        g.drawEllipse(centre.x - radius + 0.5f, centre.y - radius + 1.2f, radius * 2.0f, radius * 2.0f, thickness);

        g.setColour(rimColour);
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, thickness);

        if (rimLit)
        {
            juce::Path topArc;
            topArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                                 -juce::MathConstants<float>::pi * 0.82f,
                                 -juce::MathConstants<float>::pi * 0.18f, true);
            g.setColour(highlightColour.withAlpha(0.42f));
            g.strokePath(topArc, juce::PathStrokeType(thickness * 0.55f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
    }

    inline void fillInnerCap(juce::Graphics& g, juce::Point<float> centre, float capRadius, juce::Colour accent,
                             bool active)
    {
        juce::ColourGradient capGradient(juce::Colour(0xff2a313a), centre.x - capRadius * 0.35f,
                                         centre.y - capRadius * 0.42f, juce::Colour(0xff070a0e),
                                         centre.x + capRadius * 0.45f, centre.y + capRadius * 0.55f, false);
        capGradient.addColour(0.5, juce::Colour(0xff141a22));
        g.setGradientFill(capGradient);
        g.fillEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        juce::ColourGradient domeHighlight(juce::Colour(0xff3a4450).withAlpha(active ? 0.62f : 0.48f),
                                           centre.x - capRadius * 0.22f, centre.y - capRadius * 0.32f,
                                           juce::Colours::transparentBlack, centre.x + capRadius * 0.18f,
                                           centre.y + capRadius * 0.22f, true);
        g.setGradientFill(domeHighlight);
        g.fillEllipse(centre.x - capRadius * 0.86f, centre.y - capRadius * 0.86f, capRadius * 1.72f,
                      capRadius * 1.72f);

        g.setColour(palette::kBorderBright.withAlpha(0.75f));
        g.drawEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f,
                      juce::jmax(0.9f, capRadius * 0.04f));

        if (active)
        {
            g.setColour(accent.withAlpha(0.12f));
            g.fillEllipse(centre.x - capRadius * 1.05f, centre.y - capRadius * 1.05f, capRadius * 2.1f,
                          capRadius * 2.1f);
        }
    }

    inline void drawValueArc(juce::Graphics& g, juce::Point<float> centre, float arcRadius, float trackThickness,
                             float rotaryStartAngle, float angle, juce::Colour accent)
    {
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                            rotary::kEndAngle, true);
        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.strokePath(track, juce::PathStrokeType(trackThickness * 0.85f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(accent.withAlpha(0.20f));
        g.strokePath(value, juce::PathStrokeType(trackThickness * 1.9f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.92f));
        g.strokePath(value, juce::PathStrokeType(trackThickness * 0.52f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    inline void drawPointer(juce::Graphics& g, juce::Point<float> centre, float angle, float innerRadius,
                            float outerRadius, juce::Colour accent, float widthScale)
    {
        const juce::Point<float> direction(std::cos(angle), std::sin(angle));
        const auto pointerStart = centre + direction * innerRadius;
        const auto pointerEnd = centre + direction * outerRadius;

        g.setColour(accent.withAlpha(0.22f));
        g.drawLine({pointerStart, pointerEnd}, juce::jmax(2.2f, widthScale * 0.055f));
        g.setColour(juce::Colour(0xffeee8ff).interpolatedWith(accent, 0.42f));
        g.drawLine({pointerStart, pointerEnd}, juce::jmax(1.1f, widthScale * 0.022f));
    }

    inline void drawLedRing(juce::Graphics& g, juce::Point<float> centre, float ringRadius, juce::Colour accent,
                            bool active)
    {
        const float dotRadius = juce::jmax(1.4f, ringRadius * 0.045f);
        const float alpha = active ? 0.78f : 0.42f;
        for (int i = 0; i < 8; ++i)
        {
            const auto satelliteAngle = -juce::MathConstants<float>::halfPi
                                    + (juce::MathConstants<float>::twoPi * static_cast<float>(i) / 8.0f);
            const auto point = centre + juce::Point<float>(std::cos(satelliteAngle), std::sin(satelliteAngle))
                                         * ringRadius;
            draw::fillGlowDot(g, point, dotRadius, accent, alpha, 4);
        }
    }

    /// Concentric decked rotary — outer recess, accent middle rim, inner cap with value arc.
    /// `outerRingOnly` / `innerCapOnly` support ConcentricGlowKnob dual-parameter dials.
    inline void drawDeckedRotarySlider(juce::Graphics& g, juce::Rectangle<float> knobBounds, float proportional,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Colour accent, Size size,
                                       bool outerRingOnly, bool innerCapOnly, bool active)
    {
        juce::ignoreUnused(rotaryEndAngle);

        const float diameter = juce::jmax(16.0f, juce::jmin(knobBounds.getWidth(), knobBounds.getHeight()));
        const auto geo = computeGeometry(diameter, size);
        const auto centre = knobBounds.getCentre();
        const float angle = rotary::proportionalToAngle(proportional);

        if (geo.drawDropShadow && !outerRingOnly)
        {
            g.setColour(palette::kShadow.withAlpha(0.38f));
            g.fillEllipse(centre.x - geo.radius + 1.0f, centre.y - geo.radius + 2.5f, geo.radius * 2.0f,
                          geo.radius * 2.0f);
        }

        if (!innerCapOnly)
        {
            g.setColour(palette::kBackgroundBottom);
            g.fillEllipse(centre.x - geo.outerDeckRadius, centre.y - geo.outerDeckRadius, geo.outerDeckRadius * 2.0f,
                          geo.outerDeckRadius * 2.0f);

            strokeDeckRing(g, centre, geo.outerDeckRadius, geo.trackThickness * 0.55f,
                           palette::kBorder.withAlpha(0.55f), palette::kTopHighlight, geo.drawRimGlow);
        }

        if (outerRingOnly)
        {
            drawValueArc(g, centre, geo.outerDeckRadius * 0.94f, geo.trackThickness * 0.85f, rotaryStartAngle, angle,
                         accent);
            drawPointer(g, centre, angle, geo.outerDeckRadius * 0.78f, geo.outerDeckRadius * 0.96f, accent,
                        geo.radius);
            return;
        }

        strokeDeckRing(g, centre, geo.middleDeckRadius, geo.trackThickness * 0.72f,
                       accent.withAlpha(active ? 0.72f : 0.48f), accent.brighter(0.25f), geo.drawRimGlow);

        if (geo.drawLedRing)
            drawLedRing(g, centre, geo.middleDeckRadius, accent, active);

        drawValueArc(g, centre, geo.valueArcRadius, geo.trackThickness, rotaryStartAngle, angle, accent);

        fillInnerCap(g, centre, geo.innerCapRadius, accent, active);
        drawPointer(g, centre, angle, geo.innerCapRadius * 0.28f, geo.innerCapRadius * 0.74f, accent, geo.radius);
    }

} // namespace pw8::plugin::ui::decked
