#pragma once

#include <cmath>

#include <juce_graphics/juce_graphics.h>

#include "ObsidianDraw.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"
#include "RadialGlowDraw.h"

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
                geo.trackThickness = juce::jmax(2.4f, geo.radius * 0.058f);
                geo.drawLedRing = true;
                geo.drawDropShadow = true;
                geo.drawRimGlow = true;
                break;
            case Size::Small:
                geo.outerDeckRadius = geo.radius * 0.94f;
                geo.middleDeckRadius = geo.radius * 0.78f;
                geo.innerCapRadius = geo.radius * 0.52f;
                geo.trackThickness = juce::jmax(1.6f, geo.radius * 0.062f);
                break;
            default:
                geo.outerDeckRadius = geo.radius * 0.95f;
                geo.middleDeckRadius = geo.radius * 0.80f;
                geo.innerCapRadius = geo.radius * 0.55f;
                geo.trackThickness = juce::jmax(2.0f, geo.radius * 0.060f);
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
                             float rotaryStartAngle, float rotaryEndAngle, float angle, juce::Colour accent)
    {
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.strokePath(track, juce::PathStrokeType(trackThickness * 0.85f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(accent.withAlpha(0.20f));
        g.strokePath(value, juce::PathStrokeType(trackThickness * 2.75f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.94f));
        g.strokePath(value, juce::PathStrokeType(trackThickness * 0.78f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    inline void drawPointer(juce::Graphics& g, juce::Point<float> centre, float angle, float innerRadius,
                            float outerRadius, juce::Colour accent, float widthScale)
    {
        const juce::Point<float> direction = rotary::unitDirectionAtAngle(angle);
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
            const auto point = centre + rotary::unitDirectionAtAngle(satelliteAngle) * ringRadius;
            draw::fillGlowDot(g, point, dotRadius, accent, alpha, 4);
        }
    }

    inline void drawMinMaxTicks(juce::Graphics& g, juce::Point<float> centre, float arcRadius, float trackThickness,
                                float rotaryStartAngle, float rotaryEndAngle)
    {
        constexpr int kTickCount = 11;
        g.setColour(palette::kTextSecondary.withAlpha(0.55f));
        for (int i = 0; i < kTickCount; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kTickCount - 1);
            const float angle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * t;
            const juce::Point<float> direction = rotary::unitDirectionAtAngle(angle);
            const float inner = arcRadius - trackThickness * 0.35f;
            const float outer = arcRadius + trackThickness * 0.55f;
            const float tickWidth = (i == 0 || i == kTickCount - 1) ? 1.4f : 0.8f;
            g.drawLine({centre + direction * inner, centre + direction * outer}, tickWidth);
        }

        g.setFont(juce::Font(juce::FontOptions(juce::jmax(7.5f, arcRadius * 0.16f))));
        g.setColour(palette::kTextSecondary.withAlpha(0.72f));
        const auto minDir = rotary::unitDirectionAtAngle(rotaryStartAngle);
        const auto maxDir = rotary::unitDirectionAtAngle(rotaryEndAngle);
        const float labelRadius = arcRadius + trackThickness * 2.8f;
        g.drawText("MIN", juce::Rectangle<float>(centre + minDir * labelRadius - juce::Point<float>(14.0f, 6.0f),
                                                 juce::Point<float>(28.0f, 12.0f)),
                   juce::Justification::centred);
        g.drawText("MAX", juce::Rectangle<float>(centre + maxDir * labelRadius - juce::Point<float>(14.0f, 6.0f),
                                                 juce::Point<float>(28.0f, 12.0f)),
                   juce::Justification::centred);
    }

    inline void drawWhiteLinePointer(juce::Graphics& g, juce::Point<float> centre, float angle, float innerRadius,
                                     float outerRadius, float widthScale)
    {
        const juce::Point<float> direction = rotary::unitDirectionAtAngle(angle);
        const auto pointerStart = centre + direction * innerRadius;
        const auto pointerEnd = centre + direction * outerRadius;
        g.setColour(palette::kShadow.withAlpha(0.35f));
        g.drawLine({pointerStart, pointerEnd}, juce::jmax(2.6f, widthScale * 0.06f));
        g.setColour(juce::Colour(0xffeee8ff));
        g.drawLine({pointerStart, pointerEnd}, juce::jmax(1.6f, widthScale * 0.028f));
    }

    inline void drawDotPointer(juce::Graphics& g, juce::Point<float> centre, float angle, float orbitRadius,
                               float widthScale)
    {
        const juce::Point<float> direction = rotary::unitDirectionAtAngle(angle);
        const auto dotCentre = centre + direction * orbitRadius;
        const float dotRadius = juce::jmax(2.2f, widthScale * 0.038f);
        g.setColour(palette::kShadow.withAlpha(0.45f));
        g.fillEllipse(dotCentre.x - dotRadius + 0.4f, dotCentre.y - dotRadius + 0.6f, dotRadius * 2.0f,
                      dotRadius * 2.0f);
        g.setColour(juce::Colour(0xffeee8ff));
        g.fillEllipse(dotCentre.x - dotRadius, dotCentre.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    inline void fillColouredInnerCap(juce::Graphics& g, juce::Point<float> centre, float capRadius, juce::Colour accent,
                                     bool active)
    {
        juce::ColourGradient capGradient(accent.brighter(active ? 0.18f : 0.08f), centre.x - capRadius * 0.3f,
                                         centre.y - capRadius * 0.38f, accent.darker(0.55f), centre.x + capRadius * 0.4f,
                                         centre.y + capRadius * 0.5f, false);
        capGradient.addColour(0.45, accent.withAlpha(0.92f));
        g.setGradientFill(capGradient);
        g.fillEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        juce::ColourGradient domeHighlight(juce::Colours::white.withAlpha(active ? 0.22f : 0.14f),
                                           centre.x - capRadius * 0.22f, centre.y - capRadius * 0.32f,
                                           juce::Colours::transparentBlack, centre.x + capRadius * 0.18f,
                                           centre.y + capRadius * 0.22f, true);
        g.setGradientFill(domeHighlight);
        g.fillEllipse(centre.x - capRadius * 0.86f, centre.y - capRadius * 0.86f, capRadius * 1.72f,
                      capRadius * 1.72f);

        g.setColour(palette::kBorderBright.withAlpha(0.85f));
        g.drawEllipse(centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f,
                      juce::jmax(0.9f, capRadius * 0.045f));
    }

    constexpr float kDualKnobInnerInset = 22.0f;
    constexpr float kDualOuterArcStroke = 6.0f;

    /// Outer ring of a stacked dual knob: hollow track + structural accent value arc (Figma UX-09).
    inline void drawDualOuterRotarySlider(juce::Graphics& g, juce::Rectangle<float> knobBounds, float proportional,
                                          float rotaryStartAngle, float rotaryEndAngle, bool active,
                                          float dimAlpha = 1.0f)
    {
        juce::ignoreUnused(active);
        const float diameter = juce::jmax(16.0f, juce::jmin(knobBounds.getWidth(), knobBounds.getHeight()));
        const auto layout = radialglow::computeLayout(knobBounds.withSizeKeepingCentre(diameter, diameter));
        const float angle = rotary::proportionalToAngle(proportional, rotaryStartAngle, rotaryEndAngle);

        if (dimAlpha >= 0.99f)
            radialglow::drawBackgroundHalo(g, layout.centre, layout.outerRingRadius, juce::Colour(0xff00ffd2));
        radialglow::strokeDimTrackArc(g, layout.centre, layout.outerRingRadius, layout.outerStroke, rotaryStartAngle,
                                       rotaryEndAngle, palette::kBorder, dimAlpha);
        radialglow::strokeGlowValueArc(g, layout.centre, layout.outerRingRadius, layout.outerStroke, rotaryStartAngle,
                                       angle, juce::Colour(0xff00ffd2), dimAlpha);
    }

    /// Inner ring of a stacked dual knob: second glow arc (Figma UX-09), not a filled cap.
    inline void drawDualInnerRotarySlider(juce::Graphics& g, juce::Rectangle<float> knobBounds, float proportional,
                                          float rotaryStartAngle, float rotaryEndAngle, juce::Colour accent, bool active,
                                          float dimAlpha = 1.0f)
    {
        juce::ignoreUnused(active);
        const float diameter = juce::jmax(16.0f, juce::jmin(knobBounds.getWidth(), knobBounds.getHeight()));
        const auto layout = radialglow::computeLayout(knobBounds.withSizeKeepingCentre(diameter, diameter));
        const float angle = rotary::proportionalToAngle(proportional, rotaryStartAngle, rotaryEndAngle);
        const juce::Colour ringColour = accent.isTransparent() ? juce::Colour(0xff9f80ff) : accent;

        radialglow::strokeDimTrackArc(g, layout.centre, layout.middleRingRadius, layout.middleStroke,
                                       rotaryStartAngle, rotaryEndAngle, palette::kBorder, dimAlpha);
        radialglow::strokeGlowValueArc(g, layout.centre, layout.middleRingRadius, layout.middleStroke,
                                       rotaryStartAngle, angle, ringColour, dimAlpha);
    }

    /// Concentric decked rotary — outer recess, accent middle rim, inner cap with value arc.
    /// `outerRingOnly` / `innerCapOnly` support ConcentricGlowKnob dual-parameter dials.
    inline void drawDeckedRotarySlider(juce::Graphics& g, juce::Rectangle<float> knobBounds, float proportional,
                                       float rotaryStartAngle, float rotaryEndAngle, juce::Colour accent, Size size,
                                       bool outerRingOnly, bool innerCapOnly, bool active, bool featured = false)
    {
        const float diameter = juce::jmax(16.0f, juce::jmin(knobBounds.getWidth(), knobBounds.getHeight()));
        const auto geo = computeGeometry(diameter, size);
        const auto centre = knobBounds.getCentre();
        const float angle = rotary::proportionalToAngle(proportional, rotaryStartAngle, rotaryEndAngle);

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

            if (featured)
            {
                g.setColour(palette::kAccentWarm.withAlpha(0.07f));
                g.fillEllipse(centre.x - geo.outerDeckRadius, centre.y - geo.outerDeckRadius,
                              geo.outerDeckRadius * 2.0f, geo.outerDeckRadius * 2.0f);
            }

            strokeDeckRing(g, centre, geo.outerDeckRadius, geo.trackThickness * 0.55f,
                           featured ? palette::kAccentWarmDim.withAlpha(0.65f) : palette::kBorder.withAlpha(0.55f),
                           palette::kTopHighlight, geo.drawRimGlow || featured);
        }

        if (outerRingOnly)
        {
            drawValueArc(g, centre, geo.outerDeckRadius * 0.94f, geo.trackThickness * 0.85f, rotaryStartAngle,
                         rotaryEndAngle, angle, accent);
            drawPointer(g, centre, angle, geo.outerDeckRadius * 0.78f, geo.outerDeckRadius * 0.96f, accent,
                        geo.radius);
            return;
        }

        strokeDeckRing(g, centre, geo.middleDeckRadius, geo.trackThickness * 0.72f,
                       accent.withAlpha(active ? (featured ? 0.82f : 0.72f) : (featured ? 0.58f : 0.48f)),
                       accent.brighter(featured ? 0.32f : 0.25f), geo.drawRimGlow || featured);

        if (geo.drawLedRing)
            drawLedRing(g, centre, geo.middleDeckRadius, accent, active);

        drawValueArc(g, centre, geo.valueArcRadius, geo.trackThickness, rotaryStartAngle, rotaryEndAngle, angle,
                     accent);

        fillInnerCap(g, centre, geo.innerCapRadius, accent, active);
        drawPointer(g, centre, angle, geo.innerCapRadius * 0.28f, geo.innerCapRadius * 0.74f, accent, geo.radius);
    }

} // namespace pw8::plugin::ui::decked
