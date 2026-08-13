#pragma once

#include <juce_graphics/juce_graphics.h>

#include "EnvelopeCurveMath.h"

namespace pw8::plugin::ui::wireframe
{
    inline constexpr float kEnvelopeMinStage = 0.06f;
    inline constexpr float kEnvelopeSustainDisplay = 0.18f;

    struct EnvelopeLayout
    {
        juce::Rectangle<float> bounds;
        float totalDisplaySeconds = 0.0f;
        float delayDisplay = 0.0f;
        float attackDisplay = 0.0f;
        float holdDisplay = 0.0f;
        float decayDisplay = 0.0f;
        float sustainDisplay = kEnvelopeSustainDisplay;
        float releaseDisplay = 0.0f;

        float delayEndX = 0.0f;
        float attackPeakX = 0.0f;
        float holdEndX = 0.0f;
        float decaySustainX = 0.0f;
        float sustainEndX = 0.0f;
        float releaseEndX = 0.0f;
        float decaySustainY = 0.0f;
        float baselineY = 0.0f;

        [[nodiscard]] float displayTimeFromX(float x) const noexcept
        {
            if (bounds.getWidth() <= 0.0f || totalDisplaySeconds <= 0.0f)
                return 0.0f;
            return (x - bounds.getX()) / bounds.getWidth() * totalDisplaySeconds;
        }

        [[nodiscard]] float xFromDisplayTime(float displayTime) const noexcept
        {
            if (totalDisplaySeconds <= 0.0f)
                return bounds.getX();
            return bounds.getX() + (displayTime / totalDisplaySeconds) * bounds.getWidth();
        }

        [[nodiscard]] float levelFromY(float y) const noexcept
        {
            const float topPad = bounds.getHeight() * 0.06f;
            const float usable = bounds.getHeight() * 0.88f;
            if (usable <= 0.0f)
                return 0.0f;
            return juce::jlimit(0.0f, 1.0f, (bounds.getBottom() - topPad - y) / usable);
        }

        [[nodiscard]] float yFromLevel(float level) const noexcept
        {
            return bounds.getBottom() - level * bounds.getHeight() * 0.88f - bounds.getHeight() * 0.06f;
        }
    };

    [[nodiscard]] inline float envelopeDisplayWeightForTime(float seconds, bool allowZero) noexcept
    {
        if (allowZero && seconds <= 0.0f)
            return 0.0f;
        return juce::jmax(kEnvelopeMinStage, seconds);
    }

    [[nodiscard]] inline float envelopeTimeFromDisplayWeight(float displayWeight) noexcept
    {
        return juce::jlimit(0.0f, 60.0f, displayWeight);
    }

    [[nodiscard]] inline EnvelopeLayout computeEnvelopeLayout(const EnvelopePreviewParams& params,
                                                               juce::Rectangle<float> bounds) noexcept
    {
        EnvelopeLayout layout;
        layout.bounds = bounds;

        layout.delayDisplay = envelopeDisplayWeightForTime(params.delaySeconds, true);
        layout.attackDisplay = envelopeDisplayWeightForTime(params.attackSeconds, false);
        layout.holdDisplay = envelopeDisplayWeightForTime(params.holdSeconds, true);
        layout.decayDisplay = envelopeDisplayWeightForTime(params.decaySeconds, false);
        layout.sustainDisplay = juce::jmax(kEnvelopeSustainDisplay, kEnvelopeMinStage);
        layout.releaseDisplay = envelopeDisplayWeightForTime(params.releaseSeconds, false);

        layout.totalDisplaySeconds = layout.delayDisplay + layout.attackDisplay + layout.holdDisplay
                                     + layout.decayDisplay + layout.sustainDisplay + layout.releaseDisplay;

        layout.baselineY = layout.yFromLevel(0.0f);
        layout.delayEndX = layout.xFromDisplayTime(layout.delayDisplay);
        layout.attackPeakX = layout.xFromDisplayTime(layout.delayDisplay + layout.attackDisplay);
        layout.holdEndX = layout.xFromDisplayTime(layout.delayDisplay + layout.attackDisplay + layout.holdDisplay);
        layout.decaySustainX =
            layout.xFromDisplayTime(layout.delayDisplay + layout.attackDisplay + layout.holdDisplay + layout.decayDisplay);
        layout.sustainEndX =
            layout.xFromDisplayTime(layout.delayDisplay + layout.attackDisplay + layout.holdDisplay + layout.decayDisplay
                                    + layout.sustainDisplay);
        layout.releaseEndX = layout.xFromDisplayTime(layout.totalDisplaySeconds);
        layout.decaySustainY = layout.yFromLevel(params.sustainLevel);

        return layout;
    }

    inline void appendCurvedSegment(juce::Path& path, float x0, float x1, float y0, float y1, float curveShape,
                                    bool shapeUp, int steps)
    {
        path.lineTo(x0, y0);
        for (int i = 1; i <= steps; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(steps);
            const float shaped = shapeUp ? envelopeShapeUp(t, curveShape) : envelopeShapeDown(t, curveShape);
            const float y = y0 + (y1 - y0) * shaped;
            const float x = x0 + (x1 - x0) * t;
            path.lineTo(x, y);
        }
    }

    inline void buildEnvelopePath(const EnvelopePreviewParams& params, juce::Rectangle<float> bounds, juce::Path& outline,
                                  juce::Path& fillPath, float& sustainEndX)
    {
        const auto layout = computeEnvelopeLayout(params, bounds);
        const auto xFor = [&](float stageStart) { return layout.xFromDisplayTime(stageStart); };
        const auto yFor = [&](float level) { return layout.yFromLevel(level); };

        outline.clear();
        fillPath.clear();

        float cursor = 0.0f;
        const float yZero = yFor(0.0f);
        outline.startNewSubPath(xFor(cursor), yZero);

        if (layout.delayDisplay > 0.0f)
        {
            const float x1 = xFor(cursor + layout.delayDisplay);
            outline.lineTo(x1, yZero);
            cursor += layout.delayDisplay;
        }

        {
            const float x0 = xFor(cursor);
            const float x1 = xFor(cursor + layout.attackDisplay);
            appendCurvedSegment(outline, x0, x1, yFor(0.0f), yFor(1.0f), params.curveShape, true, 32);
            cursor += layout.attackDisplay;
        }

        if (layout.holdDisplay > 0.0f)
        {
            const float x1 = xFor(cursor + layout.holdDisplay);
            outline.lineTo(x1, yFor(1.0f));
            cursor += layout.holdDisplay;
        }

        {
            const float x0 = xFor(cursor);
            const float x1 = xFor(cursor + layout.decayDisplay);
            appendCurvedSegment(outline, x0, x1, yFor(1.0f), yFor(params.sustainLevel), params.curveShape, false, 32);
            cursor += layout.decayDisplay;
        }

        sustainEndX = xFor(cursor + layout.sustainDisplay);
        outline.lineTo(sustainEndX, yFor(params.sustainLevel));

        {
            const float x0 = sustainEndX;
            const float x1 = xFor(cursor + layout.sustainDisplay + layout.releaseDisplay);
            appendCurvedSegment(outline, x0, x1, yFor(params.sustainLevel), yFor(0.0f), params.curveShape, false, 32);
        }

        fillPath = outline;
        fillPath.lineTo(outline.getBounds().getRight(), bounds.getBottom());
        fillPath.lineTo(bounds.getX(), bounds.getBottom());
        fillPath.closeSubPath();
    }

} // namespace pw8::plugin::ui::wireframe
