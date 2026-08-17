#include "EngineIconGrid.h"

#include <cmath>

namespace pw8::plugin::ui::engineicons
{
    namespace
    {
        juce::Path classicPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float midY = b.getCentreY();
            const float amp = b.getHeight() * 0.32f;
            p.startNewSubPath(b.getX(), midY);
            for (int i = 0; i <= 24; ++i)
            {
                const float t = static_cast<float>(i) / 24.0f;
                const float x = b.getX() + t * b.getWidth();
                const float y = midY - amp * std::sin(t * juce::MathConstants<float>::twoPi);
                p.lineTo(x, y);
            }
            return p;
        }

        juce::Path wavetablePath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float inset = b.getWidth() * 0.08f;
            for (int i = 0; i < 3; ++i)
            {
                const float y = b.getY() + b.getHeight() * (0.28f + static_cast<float>(i) * 0.22f);
                p.startNewSubPath(b.getX() + inset, y);
                p.lineTo(b.getRight() - inset, y);
            }
            return p;
        }

        juce::Path fmPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float r = b.getWidth() * 0.22f;
            const auto c1 = juce::Point<float>(b.getCentreX() - r * 0.55f, b.getCentreY());
            const auto c2 = juce::Point<float>(b.getCentreX() + r * 0.55f, b.getCentreY());
            p.addEllipse(c1.x - r, c1.y - r, r * 2.0f, r * 2.0f);
            p.addEllipse(c2.x - r, c2.y - r, r * 2.0f, r * 2.0f);
            return p;
        }

        juce::Path additivePath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float baseY = b.getBottom() - b.getHeight() * 0.15f;
            const float heights[] = {0.35f, 0.55f, 0.75f, 0.45f};
            for (int i = 0; i < 4; ++i)
            {
                const float x = b.getX() + b.getWidth() * (0.15f + static_cast<float>(i) * 0.22f);
                const float h = b.getHeight() * heights[static_cast<std::size_t>(i)];
                p.startNewSubPath(x, baseY);
                p.lineTo(x, baseY - h);
            }
            return p;
        }

        juce::Path phaseShapePath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float y0 = b.getBottom() - b.getHeight() * 0.2f;
            const float y1 = b.getY() + b.getHeight() * 0.2f;
            p.startNewSubPath(b.getX() + b.getWidth() * 0.1f, y0);
            p.lineTo(b.getCentreX(), y1);
            p.lineTo(b.getRight() - b.getWidth() * 0.1f, y0);
            return p;
        }

        juce::Path granularPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const juce::Point<float> dots[] = {
                {b.getX() + b.getWidth() * 0.22f, b.getCentreY() - b.getHeight() * 0.12f},
                {b.getCentreX(), b.getCentreY() + b.getHeight() * 0.1f},
                {b.getRight() - b.getWidth() * 0.22f, b.getCentreY() - b.getHeight() * 0.08f},
            };
            for (const auto& d : dots)
                p.addEllipse(d.x - 1.8f, d.y - 1.8f, 3.6f, 3.6f);
            p.startNewSubPath(dots[0].x, dots[0].y);
            p.quadraticTo(b.getCentreX(), b.getY() + b.getHeight() * 0.15f, dots[2].x, dots[2].y);
            return p;
        }

        juce::Path resonatorPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float baseY = b.getBottom() - b.getHeight() * 0.18f;
            p.startNewSubPath(b.getX() + b.getWidth() * 0.08f, baseY);
            for (int i = 0; i <= 20; ++i)
            {
                const float t = static_cast<float>(i) / 20.0f;
                const float x = b.getX() + t * b.getWidth();
                const float env = std::exp(-3.5f * std::abs(t - 0.5f) * 2.0f);
                const float y = baseY - env * b.getHeight() * 0.72f;
                p.lineTo(x, y);
            }
            return p;
        }

        juce::Path noisePath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float midY = b.getCentreY();
            const float amp = b.getHeight() * 0.35f;
            const float xs[] = {0.0f, 0.18f, 0.32f, 0.48f, 0.62f, 0.78f, 1.0f};
            const float ys[] = {0.0f, 0.55f, -0.35f, 0.75f, -0.6f, 0.4f, -0.2f};
            p.startNewSubPath(b.getX(), midY);
            for (int i = 0; i < 7; ++i)
            {
                const float x = b.getX() + xs[static_cast<std::size_t>(i)] * b.getWidth();
                const float y = midY - ys[static_cast<std::size_t>(i)] * amp;
                p.lineTo(x, y);
            }
            return p;
        }
        juce::Path externalPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float inset = b.getWidth() * 0.12f;
            const float midY = b.getCentreY();
            p.startNewSubPath(b.getX() + inset, midY);
            p.lineTo(b.getCentreX(), midY);
            p.startNewSubPath(b.getCentreX(), midY - b.getHeight() * 0.18f);
            p.lineTo(b.getRight() - inset, midY);
            p.lineTo(b.getCentreX(), midY + b.getHeight() * 0.18f);
            p.closeSubPath();
            return p;
        }
    } // namespace

    juce::Path pathForEngine(algorithm::EngineType engine, juce::Rectangle<float> bounds)
    {
        switch (engine)
        {
            case algorithm::EngineType::Classic: return classicPath(bounds);
            case algorithm::EngineType::Wavetable: return wavetablePath(bounds);
            case algorithm::EngineType::FmPm: return fmPath(bounds);
            case algorithm::EngineType::Additive: return additivePath(bounds);
            case algorithm::EngineType::PhaseShape: return phaseShapePath(bounds);
            case algorithm::EngineType::Granular: return granularPath(bounds);
            case algorithm::EngineType::NoiseChaos: return noisePath(bounds);
            case algorithm::EngineType::Resonator: return resonatorPath(bounds);
            case algorithm::EngineType::External: return externalPath(bounds);
        }
        return {};
    }

    void drawEngineIcon(juce::Graphics& g, algorithm::EngineType engine, juce::Rectangle<float> bounds,
                        juce::Colour colour, float strokeWidth)
    {
        const auto path = pathForEngine(engine, bounds.reduced(bounds.getWidth() * 0.08f));
        g.setColour(colour);
        if (engine == algorithm::EngineType::External)
            g.fillPath(path);
        else
            g.strokePath(path, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

} // namespace pw8::plugin::ui::engineicons
