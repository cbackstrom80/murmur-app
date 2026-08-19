#include "FilterRoutingWireframeView.h"

#include "../../PlayModeLayout.h"
#include "state/PluginState.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                       float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        void strokePathGlow(juce::Graphics& g, const juce::Path& path, juce::Colour colour, float alpha,
                            float width = 1.5f)
        {
            g.setColour(colour.withAlpha(alpha));
            g.strokePath(path, juce::PathStrokeType(width));
        }

        void drawNode(juce::Graphics& g, juce::Rectangle<float> r, const char* label, juce::Colour colour, float alpha,
                      bool fillAccent)
        {
            g.setColour(fillAccent ? colour.withAlpha(0.08f * alpha) : palette::kBackgroundBottom);
            g.fillRoundedRectangle(r, 3.0f);
            g.setColour((fillAccent ? colour : palette::kBorder).withAlpha(alpha));
            g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);
            g.setFont(fonts::label(7.0f));
            g.setColour(fillAccent ? colour.withAlpha(alpha) : palette::kTextSecondary.withAlpha(alpha));
            g.drawText(label, r, juce::Justification::centred);
        }
    } // namespace

    FilterRoutingWireframeView::FilterRoutingWireframeView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        setCaption("FILTER ROUTING MATRIX");
        startTimerHz(12);
    }

    FilterRoutingWireframeView::~FilterRoutingWireframeView() { stopTimer(); }

    void FilterRoutingWireframeView::setStackedStatesLayout(bool stacked) noexcept
    {
        stackedStatesLayout_ = stacked;
        setCaption(stacked ? "FILTER ROUTING MATRIX" : "ROUTING");
        repaint();
    }

    void FilterRoutingWireframeView::timerCallback()
    {
        routing_ = loadParam(apvts_, kFilterRoutingId);
        f1Enabled_ = loadParam(apvts_, juce::String(kFilterIdPrefix) + "Enabled") >= 0.5f;
        f2Enabled_ = loadParam(apvts_, juce::String(kFilter2IdPrefix) + "Enabled") >= 0.5f;

        if (!stackedStatesLayout_)
        {
            const char* label = routing_ <= 0.05f ? "SERIAL F1→F2"
                                : routing_ >= 0.95f   ? "CROSSFADE"
                                                      : routing_ <= 0.55f ? "PARALLEL" : "MORPH";
            setSubCaption(label);
        }
        else
        {
            setSubCaption("DUAL SVF & LDR ENGINE");
        }
        repaint();
    }

    float FilterRoutingWireframeView::rowEmphasis(RoutingStateRow state) const noexcept
    {
        const float r = juce::jlimit(0.0f, 1.0f, routing_);
        switch (state)
        {
            case RoutingStateRow::Serial:
                return juce::jlimit(0.18f, 1.0f, 1.0f - r * 1.35f);
            case RoutingStateRow::Parallel:
                return juce::jlimit(0.18f, 1.0f, 1.0f - std::abs(r - 0.5f) * 2.1f);
            case RoutingStateRow::Crossfade:
                return juce::jlimit(0.18f, 1.0f, r);
        }
        return 0.2f;
    }

    void FilterRoutingWireframeView::paintStateRow(juce::Graphics& g, juce::Rectangle<float> rowBounds,
                                                    RoutingStateRow state, float emphasis) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.55f + emphasis * 0.25f));
        g.fillRoundedRectangle(rowBounds, 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f + emphasis * 0.45f));
        g.drawRoundedRectangle(rowBounds.reduced(0.5f), 6.0f, 1.0f);

        auto inner = rowBounds.reduced(8.0f, 4.0f);
        auto labelCol = inner.removeFromLeft(layout::kFilterRoutingWireframeLabelWidth);

        g.setFont(fonts::label(10.0f));
        const juce::Colour labelAccent =
            state == RoutingStateRow::Parallel ? palette::kAccent : palette::kTextPrimary;
        g.setColour(labelAccent.withAlpha(0.45f + emphasis * 0.55f));
        g.drawText(state == RoutingStateRow::Serial     ? "0.0"
                   : state == RoutingStateRow::Parallel ? "0.5"
                                                          : "1.0",
                   labelCol.removeFromTop(14.0f), juce::Justification::centredLeft);

        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kTextDim.withAlpha(0.45f + emphasis * 0.55f));
        g.drawText(state == RoutingStateRow::Serial     ? "SERIAL"
                   : state == RoutingStateRow::Parallel ? "PARALLEL"
                                                          : "CROSSFADE",
                   labelCol, juce::Justification::centredLeft);

        const float y = inner.getCentreY();
        const float nodeW = 34.0f;
        const float nodeH = 16.0f;
        const float x0 = inner.getX() + 10.0f;
        const float x1 = inner.getX() + 64.0f;
        const float x2 = inner.getX() + 142.0f;
        const float x3 = inner.getRight() - 10.0f - nodeW;

        const float pathAlpha = emphasis * (f1Enabled_ || f2Enabled_ ? 1.0f : 0.35f);

        if (state == RoutingStateRow::Serial)
        {
            juce::Path serial;
            serial.startNewSubPath(x0 + nodeW, y);
            serial.lineTo(x1, y);
            serial.lineTo(x1 + nodeW, y);
            serial.lineTo(x2, y);
            serial.lineTo(x2 + nodeW, y);
            serial.lineTo(x3, y);
            strokePathGlow(g, serial, palette::kAccent, pathAlpha);

            drawNode(g, {x0, y - nodeH * 0.5f, nodeW, nodeH}, "IN", palette::kTextDim, emphasis, false);
            if (f1Enabled_)
                drawNode(g, {x1, y - nodeH * 0.5f, nodeW, nodeH}, "F1 SVF", palette::kAccent, pathAlpha, true);
            if (f2Enabled_)
                drawNode(g, {x2, y - nodeH * 0.5f, nodeW, nodeH}, "F2 LDR", palette::kAccentWarm, pathAlpha, true);
            drawNode(g, {x3, y - nodeH * 0.5f, nodeW, nodeH}, "OUT", palette::kTextDim, emphasis, false);
        }
        else if (state == RoutingStateRow::Parallel)
        {
            juce::Path upper;
            upper.startNewSubPath(x0 + nodeW * 0.5f, y);
            upper.lineTo(x0 + nodeW + 12.0f, y - 12.0f);
            upper.lineTo(x1, y - 12.0f);
            upper.lineTo(x1 + nodeW, y - 12.0f);
            upper.lineTo(x2 - 8.0f, y - 12.0f);
            upper.lineTo(x2 + nodeW * 0.5f, y - 12.0f);
            upper.lineTo(x3 + nodeW * 0.5f, y);
            strokePathGlow(g, upper, palette::kAccent, pathAlpha * 0.9f);

            juce::Path lower;
            lower.startNewSubPath(x0 + nodeW * 0.5f, y);
            lower.lineTo(x0 + nodeW + 12.0f, y + 12.0f);
            lower.lineTo(x1, y + 12.0f);
            lower.lineTo(x1 + nodeW, y + 12.0f);
            lower.lineTo(x2 - 8.0f, y + 12.0f);
            lower.lineTo(x2 + nodeW * 0.5f, y + 12.0f);
            lower.lineTo(x3 + nodeW * 0.5f, y);
            strokePathGlow(g, lower, palette::kAccentWarm, pathAlpha * 0.85f);

            drawNode(g, {x0, y - nodeH * 0.5f, nodeW, nodeH}, "IN", palette::kTextDim, emphasis, false);
            if (f1Enabled_)
                drawNode(g, {x1, y - 12.0f - nodeH * 0.5f, nodeW, nodeH}, "F1 SVF", palette::kAccent, pathAlpha,
                         true);
            if (f2Enabled_)
                drawNode(g, {x1, y + 12.0f - nodeH * 0.5f, nodeW, nodeH}, "F2 LDR", palette::kAccentWarm, pathAlpha,
                         true);
            drawNode(g, {x3, y - nodeH * 0.5f, nodeW, nodeH}, "OUT", palette::kTextDim, emphasis, false);
        }
        else
        {
            juce::Path ghost;
            ghost.startNewSubPath(x0 + nodeW, y);
            ghost.lineTo(x1, y);
            ghost.lineTo(x2, y);
            strokePathGlow(g, ghost, palette::kTextDim, pathAlpha * 0.35f, 1.0f);

            juce::Path active;
            active.startNewSubPath(x0 + nodeW, y);
            active.lineTo(x1, y);
            active.lineTo(x1 + nodeW, y);
            active.lineTo(x2, y);
            active.lineTo(x2 + nodeW, y);
            active.lineTo(x3, y);
            strokePathGlow(g, active, palette::kAccent, pathAlpha);

            juce::Path cross;
            cross.startNewSubPath(x1 + nodeW * 0.5f, y);
            cross.lineTo(x2, y + 10.0f);
            cross.lineTo(x2 + nodeW * 0.5f, y);
            strokePathGlow(g, cross, palette::kAccentWarm.withAlpha(0.85f), pathAlpha * 0.75f, 1.2f);

            drawNode(g, {x0, y - nodeH * 0.5f, nodeW, nodeH}, "IN", palette::kTextDim, emphasis, false);
            if (f1Enabled_)
                drawNode(g, {x1, y - nodeH * 0.5f, nodeW, nodeH}, "F1 SVF", palette::kAccent, pathAlpha, true);
            if (f2Enabled_)
                drawNode(g, {x2, y - nodeH * 0.5f, nodeW, nodeH}, "F2 LDR", palette::kAccentWarm, pathAlpha, true);
            drawNode(g, {x3, y - nodeH * 0.5f, nodeW, nodeH}, "OUT", palette::kTextDim, emphasis, false);
        }
    }

    void FilterRoutingWireframeView::paintStackedStates(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        if (!f1Enabled_ && !f2Enabled_)
        {
            paintEmptyMessage(g, bounds, "Filters off");
            return;
        }

        const float rowH = layout::kFilterRoutingWireframeStateRowHeight;
        const float gap = layout::kFilterRoutingWireframeStateGap;
        auto row = bounds.removeFromTop(rowH);
        paintStateRow(g, row, RoutingStateRow::Serial, rowEmphasis(RoutingStateRow::Serial));
        bounds.removeFromTop(gap);
        row = bounds.removeFromTop(rowH);
        paintStateRow(g, row, RoutingStateRow::Parallel, rowEmphasis(RoutingStateRow::Parallel));
        bounds.removeFromTop(gap);
        row = bounds.removeFromTop(rowH);
        paintStateRow(g, row, RoutingStateRow::Crossfade, rowEmphasis(RoutingStateRow::Crossfade));
    }

    void FilterRoutingWireframeView::paintCompactMorph(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        if (!f1Enabled_ && !f2Enabled_)
        {
            paintEmptyMessage(g, bounds, "Filters off");
            return;
        }

        const float nodeW = 28.0f;
        const float nodeH = 18.0f;
        const float y = bounds.getCentreY();
        const float x0 = bounds.getX() + 8.0f;
        const float x1 = bounds.getX() + bounds.getWidth() * 0.32f;
        const float x2 = bounds.getX() + bounds.getWidth() * 0.62f;
        const float x3 = bounds.getRight() - 8.0f - nodeW;

        const float serialAlpha = juce::jlimit(0.15f, 1.0f, 1.0f - routing_ * 1.2f);
        const float parallelAlpha = juce::jlimit(0.15f, 1.0f, 1.0f - std::abs(routing_ - 0.5f) * 2.2f);
        const float crossAlpha = juce::jlimit(0.15f, 1.0f, routing_);

        juce::Path serial;
        serial.startNewSubPath(x0 + nodeW, y);
        serial.lineTo(x1, y);
        serial.lineTo(x1 + nodeW, y);
        serial.lineTo(x2, y);
        serial.lineTo(x2 + nodeW, y);
        serial.lineTo(x3, y);
        strokePathGlow(g, serial, palette::kAccent, serialAlpha);

        if (f1Enabled_ && f2Enabled_ && routing_ > 0.15f)
        {
            juce::Path parallel;
            parallel.startNewSubPath(x0 + nodeW * 0.5f, y);
            parallel.lineTo(x1, y - 10.0f);
            parallel.lineTo(x2 + nodeW * 0.5f, y - 10.0f);
            parallel.lineTo(x3, y);
            strokePathGlow(g, parallel, palette::kAccentWarm, parallelAlpha * 0.85f);

            if (routing_ > 0.55f)
            {
                juce::Path reverse;
                reverse.startNewSubPath(x0 + nodeW * 0.5f, y);
                reverse.lineTo(x2, y + 10.0f);
                reverse.lineTo(x1 + nodeW * 0.5f, y + 10.0f);
                reverse.lineTo(x3, y);
                strokePathGlow(g, reverse, palette::kAccentWarm.withAlpha(0.7f), crossAlpha * 0.65f);
            }
        }

        drawNode(g, {x0, y - nodeH * 0.5f, nodeW, nodeH}, "IN", palette::kTextDim, 0.9f, false);
        if (f1Enabled_)
            drawNode(g, {x1, y - nodeH * 0.5f, nodeW, nodeH}, "F1", palette::kAccent, serialAlpha, true);
        if (f2Enabled_)
            drawNode(g, {x2, y - nodeH * 0.5f, nodeW, nodeH}, "F2", palette::kAccentWarm,
                     juce::jmax(serialAlpha, parallelAlpha), true);
        drawNode(g, {x3, y - nodeH * 0.5f, nodeW, nodeH}, "OUT", palette::kTextDim, 0.9f, false);
    }

    void FilterRoutingWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds().reduced(2.0f);
        paintSubCaption(g, bounds);

        if (stackedStatesLayout_)
            paintStackedStates(g, bounds);
        else
            paintCompactMorph(g, bounds);
    }

} // namespace pw8::plugin::ui::wireframe
