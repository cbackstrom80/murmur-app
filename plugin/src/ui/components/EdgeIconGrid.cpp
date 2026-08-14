#include "EdgeIconGrid.h"

#include <cmath>

namespace pw8::plugin::ui::edgeicons
{
    namespace
    {
        juce::Path audioPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float y = b.getCentreY();
            p.startNewSubPath(b.getX() + b.getWidth() * 0.08f, y);
            p.lineTo(b.getRight() - b.getWidth() * 0.08f, y);
            p.startNewSubPath(b.getCentreX(), y - b.getHeight() * 0.22f);
            p.lineTo(b.getCentreX(), y + b.getHeight() * 0.22f);
            return p;
        }

        juce::Path phaseModPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float midY = b.getCentreY();
            const float amp = b.getHeight() * 0.28f;
            p.startNewSubPath(b.getX(), midY);
            for (int i = 0; i <= 16; ++i)
            {
                const float t = static_cast<float>(i) / 16.0f;
                const float x = b.getX() + t * b.getWidth();
                const float y = midY - amp * std::sin(t * juce::MathConstants<float>::twoPi * 1.5f);
                p.lineTo(x, y);
            }
            return p;
        }

        juce::Path frequencyModPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float y0 = b.getBottom() - b.getHeight() * 0.18f;
            const float y1 = b.getY() + b.getHeight() * 0.18f;
            p.startNewSubPath(b.getX() + b.getWidth() * 0.1f, y0);
            p.lineTo(b.getCentreX(), y1);
            p.lineTo(b.getRight() - b.getWidth() * 0.1f, y0);
            return p;
        }

        juce::Path amplitudeModPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float baseY = b.getCentreY();
            const float heights[] = {0.2f, 0.45f, 0.7f, 0.35f};
            for (int i = 0; i < 4; ++i)
            {
                const float x = b.getX() + b.getWidth() * (0.12f + static_cast<float>(i) * 0.24f);
                const float h = b.getHeight() * heights[static_cast<std::size_t>(i)];
                p.startNewSubPath(x, baseY + h * 0.5f);
                p.lineTo(x, baseY - h);
            }
            return p;
        }

        juce::Path ringModPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float r = b.getWidth() * 0.18f;
            const auto c1 = juce::Point<float>(b.getCentreX() - r * 0.6f, b.getCentreY());
            const auto c2 = juce::Point<float>(b.getCentreX() + r * 0.6f, b.getCentreY());
            p.addEllipse(c1.x - r, c1.y - r, r * 2.0f, r * 2.0f);
            p.addEllipse(c2.x - r, c2.y - r, r * 2.0f, r * 2.0f);
            p.startNewSubPath(c1.x + r, c1.y);
            p.lineTo(c2.x - r, c2.y);
            return p;
        }

        juce::Path syncPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const float y = b.getCentreY();
            p.startNewSubPath(b.getX() + b.getWidth() * 0.08f, y);
            p.lineTo(b.getRight() - b.getWidth() * 0.08f, y);
            for (int i = 0; i < 3; ++i)
            {
                const float x = b.getX() + b.getWidth() * (0.25f + static_cast<float>(i) * 0.25f);
                p.startNewSubPath(x, y - b.getHeight() * 0.22f);
                p.lineTo(x, y + b.getHeight() * 0.22f);
            }
            return p;
        }

        juce::Path feedbackPath(juce::Rectangle<float> b)
        {
            juce::Path p;
            const auto c = b.getCentre().translated(b.getWidth() * 0.12f, 0.0f);
            const float r = b.getWidth() * 0.22f;
            p.addArc(c.x - r, c.y - r, r * 2.0f, r * 2.0f, -juce::MathConstants<float>::halfPi,
                      juce::MathConstants<float>::pi * 1.35f);
            const auto tip = p.getCurrentPosition();
            p.startNewSubPath(tip.x, tip.y);
            p.lineTo(tip.x - 4.0f, tip.y - 3.0f);
            p.startNewSubPath(tip.x, tip.y);
            p.lineTo(tip.x + 2.0f, tip.y - 5.0f);
            return p;
        }
    } // namespace

    juce::Path pathForEdge(algorithm::EdgeType type, juce::Rectangle<float> bounds)
    {
        switch (type)
        {
            case algorithm::EdgeType::Audio: return audioPath(bounds);
            case algorithm::EdgeType::PhaseMod: return phaseModPath(bounds);
            case algorithm::EdgeType::FrequencyMod: return frequencyModPath(bounds);
            case algorithm::EdgeType::AmplitudeMod: return amplitudeModPath(bounds);
            case algorithm::EdgeType::RingMod: return ringModPath(bounds);
            case algorithm::EdgeType::Sync: return syncPath(bounds);
            case algorithm::EdgeType::Feedback: return feedbackPath(bounds);
        }
        return {};
    }

    void drawEdgeIcon(juce::Graphics& g, algorithm::EdgeType type, juce::Rectangle<float> bounds, juce::Colour colour,
                      float strokeWidth)
    {
        const auto path = pathForEdge(type, bounds.reduced(bounds.getWidth() * 0.06f));
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

} // namespace pw8::plugin::ui::edgeicons
