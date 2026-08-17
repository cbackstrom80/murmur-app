#pragma once

#include <cmath>

#include <juce_graphics/juce_graphics.h>

#include "ObsidianDraw.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"

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

    [[nodiscard]] inline Layout computeLayout(juce::Rectangle<float> bounds) noexcept
    {
        const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const float dialRadius = diameter * 0.5f;
        const auto centre = bounds.getCentre();
        Layout layout;
        layout.centre = centre;
        layout.dialRadius = dialRadius;
        layout.outerRingRadius = dialRadius * 0.889f;
        layout.middleRingRadius = dialRadius * 0.722f;
        layout.innerRingRadius = dialRadius * 0.556f;
        layout.capRadius = dialRadius * 0.444f;
        layout.outerStroke = juce::jmax(3.5f, dialRadius * 0.067f);
        layout.middleStroke = juce::jmax(3.0f, dialRadius * 0.058f);
        layout.innerStroke = juce::jmax(2.6f, dialRadius * 0.050f);
        return layout;
    }

    inline void strokeGlowValueArc(juce::Graphics& g, juce::Point<float> centre, float radius, float strokeWidth,
                                   float startAngle, float endAngle, juce::Colour colour, float dimAlpha = 1.0f)
    {
        if (endAngle <= startAngle + 0.002f)
            return;

        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);

        g.setColour(colour.withAlpha(0.18f * dimAlpha));
        g.strokePath(arc, juce::PathStrokeType(strokeWidth * 2.1f, juce::PathStrokeType::curved,
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

        g.setColour(palette::kBackgroundTop);
        g.fillEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawEllipse(c.x - r, c.y - r, r * 2.0f, r * 2.0f, juce::jmax(1.0f, r * 0.04f));
    }

    inline void drawCenterReadout(juce::Graphics& g, const Layout& layout, const juce::String& title,
                                const juce::String& value, juce::Colour accent)
    {
        fillCenterCap(g, layout);
        auto cap = juce::Rectangle<float>(layout.centre.x - layout.capRadius, layout.centre.y - layout.capRadius,
                                          layout.capRadius * 2.0f, layout.capRadius * 2.0f);

        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(8.0f, layout.capRadius * 0.22f))).boldened());
        g.drawText(title.toUpperCase(), cap.removeFromTop(cap.getHeight() * 0.38f), juce::Justification::centred);

        g.setColour(palette::kTextPrimary);
        g.setFont(juce::Font(juce::FontOptions(juce::jmax(11.0f, layout.capRadius * 0.38f))).boldened());
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

    inline void drawDoubleRadialFrame(juce::Graphics& g, juce::Rectangle<float> bounds, float outerProportional,
                                      float innerProportional, juce::Colour outerColour, juce::Colour innerColour,
                                      int focusedChannel, bool expandedReadout, const juce::String& outerValueText,
                                      const juce::String& innerValueText, const juce::String& outerShort,
                                      const juce::String& innerShort)
    {
        const auto layout = computeLayout(bounds);
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
        const auto layout = computeLayout(bounds);
        const juce::Colour cOuter{0xff00ffd2};
        const juce::Colour cMiddle{0xff9f80ff};
        const juce::Colour cInner{0xffff9d00};

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
        const auto layout = computeLayout(bounds);
        const float dist = localPos.getDistanceFrom(layout.centre);
        if (dist <= layout.capRadius)
            return -1;
        if (dist <= layout.middleRingRadius + layout.middleStroke)
            return 1;
        return 0;
    }

    [[nodiscard]] inline int hitTestTripleRingChannel(juce::Point<float> localPos, juce::Rectangle<float> bounds) noexcept
    {
        const auto layout = computeLayout(bounds);
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
