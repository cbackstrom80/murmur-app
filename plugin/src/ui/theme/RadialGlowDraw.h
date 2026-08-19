#pragma once

#include <cmath>

#include <juce_graphics/juce_graphics.h>

#include "ObsidianDraw.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"
#include "FigmaKnobTokens.h"

namespace pw8::plugin::ui::radialglow
{
    /// Figma UX-09 / glow-ring-knobs reference geometry (180px dial frame).
    struct Layout
    {
        juce::Point<float> centre{};
        float dialRadius = 0.0f;
        float outerRingRadius = 0.0f;
        float middleRingRadius = 0.0f;
        float innerRingRadius = 0.0f;
        float capRadius = 0.0f;
        float outerStroke = 6.0f;
        float middleStroke = 5.0f;
        float innerStroke = 4.5f;
    };

    [[nodiscard]] inline Layout computeDualLayout(juce::Rectangle<float> bounds) noexcept
    {
        const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const float dialRadius = diameter * 0.5f;
        const auto centre = bounds.getCentre();
        Layout layout;
        layout.centre = centre;
        layout.dialRadius = dialRadius;
        layout.outerRingRadius = dialRadius * figma::DualRingRatios::outer;
        layout.middleRingRadius = dialRadius * figma::DualRingRatios::middle;
        layout.innerRingRadius = dialRadius * figma::DualRingRatios::middle;
        layout.capRadius = dialRadius * figma::DualRingRatios::cap;
        layout.outerStroke = figma::scaledHeroStroke(figma::HeroStrokeAt180::dualOuter, diameter);
        layout.middleStroke = figma::scaledHeroStroke(figma::HeroStrokeAt180::dualMiddle, diameter);
        layout.innerStroke = figma::scaledHeroStroke(figma::HeroStrokeAt180::dualMiddle, diameter);
        return layout;
    }

    [[nodiscard]] inline Layout computeTripleLayout(juce::Rectangle<float> bounds) noexcept
    {
        const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const float dialRadius = diameter * 0.5f;
        const auto centre = bounds.getCentre();
        Layout layout;
        layout.centre = centre;
        layout.dialRadius = dialRadius;
        layout.outerRingRadius = dialRadius * figma::TripleRingRatios::outer;
        layout.middleRingRadius = dialRadius * figma::TripleRingRatios::middle;
        layout.innerRingRadius = dialRadius * figma::TripleRingRatios::inner;
        layout.capRadius = dialRadius * figma::TripleRingRatios::cap;
        layout.outerStroke = figma::scaledHeroStroke(figma::HeroStrokeAt180::tripleOuter, diameter);
        layout.middleStroke = figma::scaledHeroStroke(figma::HeroStrokeAt180::tripleMiddle, diameter);
        layout.innerStroke = figma::scaledHeroStroke(figma::HeroStrokeAt180::tripleInner, diameter);
        return layout;
    }

    [[nodiscard]] inline Layout computeLayout(juce::Rectangle<float> bounds) noexcept
    {
        return computeDualLayout(bounds);
    }

    inline void strokeGlowValueArc(juce::Graphics& g, juce::Point<float> centre, float radius, float strokeWidth,
                                   float startAngle, float endAngle, juce::Colour colour, float dimAlpha = 1.0f)
    {
        if (endAngle <= startAngle + 0.002f)
            return;

        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);

        g.setColour(colour.withAlpha(0.16f * dimAlpha));
        g.strokePath(arc, juce::PathStrokeType(strokeWidth * 1.55f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        g.setColour(colour.withAlpha(0.95f * dimAlpha));
        g.strokePath(arc, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    inline void strokeDimTrackArc(juce::Graphics& g, juce::Point<float> centre, float radius, float strokeWidth,
                                  float startAngle, float endAngle, juce::Colour trackColour, float dimAlpha = 1.0f)
    {
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour(trackColour.withAlpha(0.55f * dimAlpha));
        g.strokePath(track, juce::PathStrokeType(strokeWidth * 0.72f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    inline void drawRingChannel(juce::Graphics& g, const Layout& layout, float radius, float strokeWidth,
                                float proportional, juce::Colour colour, float dimAlpha = 1.0f)
    {
        const float start = rotary::kStartAngle;
        const float end = rotary::kEndAngle;
        const float valueAngle = rotary::proportionalToAngle(proportional, start, end);

        strokeDimTrackArc(g, layout.centre, radius, strokeWidth, start, end, palette::kBorder, dimAlpha);
        strokeGlowValueArc(g, layout.centre, radius, strokeWidth, start, valueAngle, colour, dimAlpha);
    }

    inline void fillCenterCap(juce::Graphics& g, const Layout& layout)
    {
        const auto& c = layout.centre;
        const float r = layout.capRadius;

        juce::ColourGradient capGradient(juce::Colour(0xff0c1014), c.x - r * 0.25f, c.y - r * 0.35f,
                                         juce::Colour(0xff06080b), c.x + r * 0.35f, c.y + r * 0.45f, false);
        capGradient.addColour(0.55, juce::Colour(0xff10151a));
        g.setGradientFill(capGradient);
        g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f, juce::jmax(0.8f, r * 0.025f));
    }

    inline void drawCenterReadout(juce::Graphics& g, const Layout& layout, const juce::String& title,
                                const juce::String& value, juce::Colour accent)
    {
        fillCenterCap(g, layout);
        auto cap = juce::Rectangle<float>(layout.centre.x - layout.capRadius, layout.centre.y - layout.capRadius,
                                          layout.capRadius * 2.0f, layout.capRadius * 2.0f);

        g.setColour(accent.withAlpha(0.88f));
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(7.0f, layout.capRadius * 0.16f))).boldened());
        g.drawText(title.toUpperCase(), cap.removeFromTop(cap.getHeight() * 0.32f), juce::Justification::centred);

        g.setColour(palette::kTextPrimary);
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(12.0f, layout.capRadius * 0.44f))).boldened());
        g.drawText(value, cap, juce::Justification::centred);
    }

    inline void drawStackedCenterReadout(juce::Graphics& g, const Layout& layout,
                                         const juce::String& line1, const juce::String& line2,
                                         const juce::String& line3, juce::Colour c1, juce::Colour c2, juce::Colour c3)
    {
        fillCenterCap(g, layout);
        auto cap = juce::Rectangle<float>(layout.centre.x - layout.capRadius, layout.centre.y - layout.capRadius,
                                          layout.capRadius * 2.0f, layout.capRadius * 2.0f);
        const float lineH = cap.getHeight() / 3.2f;
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(7.5f, layout.capRadius * 0.18f))).boldened());
        g.setColour(c1);
        g.drawText(line1, cap.removeFromTop(lineH), juce::Justification::centred);
        g.setColour(c2);
        g.drawText(line2, cap.removeFromTop(lineH), juce::Justification::centred);
        g.setColour(c3);
        g.drawText(line3, cap, juce::Justification::centred);
    }

    inline void drawBackgroundHalo(juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour accent)
    {
        juce::ColourGradient halo(accent.withAlpha(0.14f), centre.x, centre.y - radius * 0.15f,
                                  juce::Colours::transparentBlack, centre.x, centre.y + radius * 1.05f, true);
        g.setGradientFill(halo);
        g.fillEllipse(centre.x - radius * 1.08f, centre.y - radius * 1.08f, radius * 2.16f, radius * 2.16f);
    }

    /// Figma `21:4` single-ring performance KOIN (desktop PLAY macros @ 92px).
    inline void drawSinglePerformanceKnob(juce::Graphics& g, juce::Rectangle<float> bounds, float proportional,
                                            float rotaryStartAngle, float rotaryEndAngle, juce::Colour accent,
                                            bool featured, const juce::String& valueText, bool active)
    {
        const float diameter = juce::jmax(16.0f, juce::jmin(bounds.getWidth(), bounds.getHeight()));
        const auto layout = computeLayout(bounds.withSizeKeepingCentre(diameter, diameter));
        juce::Colour ringColour = accent.isTransparent()
                                      ? (featured ? palette::kAccentWarm : figma::kKoinRingOuter)
                                      : accent;
        if (featured)
            drawBackgroundHalo(g, layout.centre, layout.outerRingRadius, ringColour);

        drawRingChannel(g, layout, layout.outerRingRadius, layout.outerStroke, proportional, ringColour,
                        active ? 1.0f : 0.92f);
        fillCenterCap(g, layout);

        auto cap = juce::Rectangle<float>(layout.centre.x - layout.capRadius, layout.centre.y - layout.capRadius,
                                          layout.capRadius * 2.0f, layout.capRadius * 2.0f);
        g.setColour(palette::kTextPrimary);
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(11.0f, layout.capRadius * 0.34f))).boldened());
        g.drawText(valueText, cap, juce::Justification::centred);

        const float angle = rotary::proportionalToAngle(proportional, rotaryStartAngle, rotaryEndAngle);
        const juce::Point<float> direction = rotary::unitDirectionAtAngle(angle);
        const auto dotCentre = layout.centre + direction * layout.outerRingRadius;
        const float dotRadius = juce::jmax(2.0f, layout.outerStroke * 0.34f);
        g.setColour(ringColour.withAlpha(0.35f));
        g.fillEllipse(dotCentre.x - dotRadius * 1.35f, dotCentre.y - dotRadius * 1.35f, dotRadius * 2.7f,
                      dotRadius * 2.7f);
        g.setColour(juce::Colour(0xffeee8ff));
        g.fillEllipse(dotCentre.x - dotRadius, dotCentre.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    inline void drawDoubleRadialFrame(juce::Graphics& g, juce::Rectangle<float> bounds, float outerProportional,
                                      float innerProportional, juce::Colour outerColour, juce::Colour innerColour,
                                      int focusedChannel, bool expandedReadout, const juce::String& outerValueText,
                                      const juce::String& innerValueText, const juce::String& outerShort,
                                      const juce::String& innerShort)
    {
        const auto layout = computeDualLayout(bounds);
        drawBackgroundHalo(g, layout.centre, layout.outerRingRadius, outerColour);

        const float outerDim = (focusedChannel == 1) ? 0.35f : 1.0f;
        const float innerDim = (focusedChannel == 0) ? 0.35f : 1.0f;

        drawRingChannel(g, layout, layout.outerRingRadius, layout.outerStroke, outerProportional, outerColour, outerDim);
        drawRingChannel(g, layout, layout.middleRingRadius, layout.middleStroke, innerProportional, innerColour,
                        innerDim);

        if (expandedReadout)
            drawStackedCenterReadout(g, layout, outerValueText, innerValueText, juce::String(), outerColour,
                                     innerColour, juce::Colours::transparentBlack);
        else if (focusedChannel == 1)
            drawCenterReadout(g, layout, innerShort, innerValueText, innerColour);
        else
            drawCenterReadout(g, layout, outerShort, outerValueText, outerColour);
    }

    inline void drawTripleRadialFrame(juce::Graphics& g, juce::Rectangle<float> bounds, float outerProportional,
                                      float middleProportional, float innerProportional, int focusedChannel,
                                      bool expandedReadout, const juce::String& v1, const juce::String& v2,
                                      const juce::String& v3, const juce::String& s1, const juce::String& s2,
                                      const juce::String& s3)
    {
        const auto layout = computeTripleLayout(bounds);
        const juce::Colour cOuter = figma::kKoinRingOuter;
        const juce::Colour cMiddle = figma::kKoinRingMiddle;
        const juce::Colour cInner = figma::kKoinRingInner;

        drawBackgroundHalo(g, layout.centre, layout.outerRingRadius, cOuter);

        drawRingChannel(g, layout, layout.outerRingRadius, layout.outerStroke, outerProportional, cOuter,
                        focusedChannel == 0 ? 1.0f : 0.35f);
        drawRingChannel(g, layout, layout.middleRingRadius, layout.middleStroke, middleProportional, cMiddle,
                        focusedChannel == 1 ? 1.0f : 0.35f);
        drawRingChannel(g, layout, layout.innerRingRadius, layout.innerStroke, innerProportional, cInner,
                        focusedChannel == 2 ? 1.0f : 0.35f);

        if (expandedReadout)
            drawStackedCenterReadout(g, layout, v1, v2, v3, cOuter, cMiddle, cInner);
        else if (focusedChannel == 1)
            drawCenterReadout(g, layout, s2, v2, cMiddle);
        else if (focusedChannel == 2)
            drawCenterReadout(g, layout, s3, v3, cInner);
        else
            drawCenterReadout(g, layout, s1, v1, cOuter);
    }

    [[nodiscard]] inline int hitTestRingChannel(juce::Point<float> localPos, juce::Rectangle<float> bounds) noexcept
    {
        const auto layout = computeDualLayout(bounds);
        const float dist = localPos.getDistanceFrom(layout.centre);
        if (dist <= layout.capRadius)
            return -1;
        if (dist <= layout.middleRingRadius + layout.middleStroke)
            return 1;
        return 0;
    }

    [[nodiscard]] inline int hitTestTripleRingChannel(juce::Point<float> localPos, juce::Rectangle<float> bounds) noexcept
    {
        const auto layout = computeTripleLayout(bounds);
        const float dist = localPos.getDistanceFrom(layout.centre);
        if (dist <= layout.capRadius)
            return -1;
        if (dist <= layout.innerRingRadius + layout.innerStroke)
            return 2;
        if (dist <= layout.middleRingRadius + layout.middleStroke)
            return 1;
        return 0;
    }

} // namespace pw8::plugin::ui::radialglow
