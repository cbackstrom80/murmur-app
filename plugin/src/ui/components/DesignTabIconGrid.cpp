#include "DesignTabIconGrid.h"

#include <cmath>
#include <functional>

namespace pw8::plugin::ui::designtabicons
{
    namespace
    {
        juce::Path normalizedPath(const std::function<void(juce::Path&, float)>& build, juce::Rectangle<float> bounds)
        {
            juce::Path raw;
            build(raw, 16.0f);
            const auto rawBounds = raw.getBounds();
            if (rawBounds.isEmpty())
                return raw;

            juce::Path scaled;
            const float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) / juce::jmax(rawBounds.getWidth(), rawBounds.getHeight());
            scaled.addPath(raw, juce::AffineTransform::scale(scale).translated(
                                        bounds.getCentreX() - rawBounds.getCentreX() * scale,
                                        bounds.getCentreY() - rawBounds.getCentreY() * scale));
            return scaled;
        }
    } // namespace

    juce::Path pathForTab(DesignTab tab, juce::Rectangle<float> bounds)
    {
        switch (tab)
        {
            case DesignTab::Graph:
                return normalizedPath(
                    [](juce::Path& p, float s)
                    {
                        const float cx = s * 0.5f;
                        const float cy = s * 0.5f;
                        const float r = s * 0.32f;
                        for (int i = 0; i < 8; ++i)
                        {
                            const float a = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 8.0f - juce::MathConstants<float>::halfPi;
                            p.addEllipse(cx + std::cos(a) * r - 1.2f, cy + std::sin(a) * r - 1.2f, 2.4f, 2.4f);
                        }
                        p.startNewSubPath(cx, cy - r * 0.35f);
                        p.lineTo(cx + r * 0.55f, cy + r * 0.25f);
                        p.lineTo(cx - r * 0.45f, cy + r * 0.35f);
                    },
                    bounds);

            case DesignTab::Matrix:
                return normalizedPath(
                    [](juce::Path& p, float s)
                    {
                        const float inset = s * 0.18f;
                        const float step = (s - inset * 2.0f) / 3.0f;
                        for (int i = 0; i <= 3; ++i)
                        {
                            const float x = inset + step * static_cast<float>(i);
                            p.startNewSubPath(x, inset);
                            p.lineTo(x, s - inset);
                            const float y = inset + step * static_cast<float>(i);
                            p.startNewSubPath(inset, y);
                            p.lineTo(s - inset, y);
                        }
                    },
                    bounds);

            case DesignTab::Fx:
                return normalizedPath(
                    [](juce::Path& p, float s)
                    {
                        const float y = s * 0.5f;
                        const float left = s * 0.15f;
                        const float right = s * 0.85f;
                        const float amp = s * 0.22f;
                        p.startNewSubPath(left, y);
                        for (int i = 0; i <= 24; ++i)
                        {
                            const float t = static_cast<float>(i) / 24.0f;
                            const float x = left + (right - left) * t;
                            const float wave = std::sin(t * juce::MathConstants<float>::twoPi * 2.5f) * amp;
                            p.lineTo(x, y + wave);
                        }
                    },
                    bounds);

            case DesignTab::Wavetable:
                return normalizedPath(
                    [](juce::Path& p, float s)
                    {
                        for (int row = 0; row < 3; ++row)
                        {
                            const float depth = static_cast<float>(row) * 0.18f;
                            const float y0 = s * (0.35f - depth);
                            const float x0 = s * (0.15f + depth);
                            const float width = s * (0.7f - depth);
                            p.startNewSubPath(x0, y0);
                            for (int i = 0; i <= 16; ++i)
                            {
                                const float t = static_cast<float>(i) / 16.0f;
                                const float x = x0 + width * t;
                                const float y = y0 + std::sin(t * juce::MathConstants<float>::twoPi) * s * 0.12f;
                                p.lineTo(x, y);
                            }
                        }
                    },
                    bounds);

            default:
                return {};
        }
    }

    void drawTabIcon(juce::Graphics& g, DesignTab tab, juce::Rectangle<float> bounds, juce::Colour colour,
                     float strokeWidth)
    {
        const auto path = pathForTab(tab, bounds);
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

} // namespace pw8::plugin::ui::designtabicons
