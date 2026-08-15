#include "FxWireframeView.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] const char* effectTypeName(int typeOrdinal) noexcept
        {
            switch (typeOrdinal)
            {
                case 0: return "BYPASS";
                case 1: return "SATURATION";
                case 2: return "CHORUS";
                case 3: return "TAPE DELAY";
                case 4: return "NODE DELAY";
                case 5: return "FREQ SHIFT ECHO";
                case 6: return "FRACTAL ECHO";
                case 7: return "REVERB";
                case 8: return "EQ";
                case 9: return "COMPRESSOR";
                case 10: return "LIMITER";
                case 11: return "VOCODER";
                default: return "?";
            }
        }

        void drawFeedbackLoop(juce::Graphics& g, juce::Rectangle<float> bounds, float mix)
        {
            const float cx = bounds.getCentreX();
            const float cy = bounds.getCentreY();
            const float w = bounds.getWidth() * 0.55f;
            const float h = bounds.getHeight() * 0.35f;

            juce::Path loop;
            loop.startNewSubPath(cx - w * 0.5f, cy);
            loop.lineTo(cx + w * 0.5f, cy);
            loop.quadraticTo(cx + w * 0.5f, cy - h, cx, cy - h);
            loop.quadraticTo(cx - w * 0.5f, cy - h, cx - w * 0.5f, cy);

            strokeGlowPath(g, loop, juce::jlimit(0.35f, 1.0f, mix + 0.25f), 2.0f, true);

            g.setColour(palette::kAccentWarm.withAlpha(0.85f));
            g.drawArrow(juce::Line<float>(cx - w * 0.15f, cy - h * 0.55f, cx - w * 0.35f, cy - h * 0.15f), 1.5f, 8.0f,
                        8.0f);
        }

        void drawNodeTree(juce::Graphics& g, juce::Rectangle<float> bounds, int nodeCount)
        {
            struct Node
            {
                float x;
                float y;
            };
            const Node nodes[] = {{0.18f, 0.55f}, {0.42f, 0.35f}, {0.62f, 0.58f}, {0.82f, 0.42f}};

            juce::Path edges;
            for (int i = 1; i < nodeCount && i < 4; ++i)
            {
                const float x0 = bounds.getX() + nodes[i - 1].x * bounds.getWidth();
                const float y0 = bounds.getY() + nodes[i - 1].y * bounds.getHeight();
                const float x1 = bounds.getX() + nodes[i].x * bounds.getWidth();
                const float y1 = bounds.getY() + nodes[i].y * bounds.getHeight();
                edges.startNewSubPath(x0, y0);
                edges.lineTo(x1, y1);
            }
            g.setColour(palette::kBorderBright.withAlpha(0.65f));
            g.strokePath(edges, juce::PathStrokeType(1.2f));

            for (int i = 0; i < nodeCount && i < 4; ++i)
            {
                const float x = bounds.getX() + nodes[i].x * bounds.getWidth();
                const float y = bounds.getY() + nodes[i].y * bounds.getHeight();
                const float r = bounds.getHeight() * 0.055f;
                g.setColour(i == 0 ? palette::kAccent : palette::kPanelRaised);
                g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);
                g.setColour(i == 0 ? palette::kAccent : palette::kBorderBright);
                g.drawEllipse(x - r, y - r, r * 2.0f, r * 2.0f, 1.4f);
            }
        }
    } // namespace

    FxWireframeView::FxWireframeView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        startTimerHz(8);
    }

    FxWireframeView::~FxWireframeView() { stopTimer(); }

    void FxWireframeView::bindToSlot(const juce::String& paramPrefix)
    {
        paramPrefix_ = paramPrefix;
        timerCallback();
    }

    void FxWireframeView::timerCallback()
    {
        if (paramPrefix_.isEmpty())
            return;

        if (auto* typeRaw = apvts_.getRawParameterValue(paramPrefix_ + "Type"))
            effectType_ = static_cast<int>(typeRaw->load() + 0.5f);
        if (auto* mixRaw = apvts_.getRawParameterValue(paramPrefix_ + "Mix"))
            mix_ = mixRaw->load();

        setCaption(juce::String(effectTypeName(effectType_)));
        setSubCaption("Mix " + juce::String(mix_, 2));
        repaint();
    }

    void FxWireframeView::paintEffectSchematic(juce::Graphics& g, juce::Rectangle<float> bounds, int type, float mix)
    {
        const bool bypassed = type == 0;
        const float alpha = bypassed ? 0.35f : juce::jlimit(0.45f, 1.0f, mix + 0.2f);

        if (bypassed)
        {
            juce::Path passthrough;
            passthrough.startNewSubPath(bounds.getX(), bounds.getCentreY());
            passthrough.lineTo(bounds.getRight(), bounds.getCentreY());
            g.setColour(palette::kBorderBright.withAlpha(0.5f));
            g.strokePath(passthrough, juce::PathStrokeType(1.5f));
            g.setFont(fonts::label(10.0f));
            g.setColour(palette::kTextDim);
            g.drawText("BYPASS", bounds, juce::Justification::centred);
            return;
        }

        switch (type)
        {
            case 1: // Saturation — waveshaper curve
                paintFlatWaveform(g, bounds, 64,
                                  [](float t)
                                  {
                                      const float x = t * 2.0f - 1.0f;
                                      return std::tanh(x * 2.2f);
                                  },
                                  true);
                break;

            case 2: // Chorus — dual detuned waves
            {
                auto inner = bounds.reduced(bounds.getWidth() * 0.08f, bounds.getHeight() * 0.12f);
                paintFlatWaveform(g, inner.removeFromTop(inner.getHeight() * 0.45f), 48,
                                  [](float t) { return std::sin(t * juce::MathConstants<float>::twoPi * 2.0f) * 0.55f; },
                                  true);
                paintFlatWaveform(g, inner, 48,
                                  [](float t)
                                  {
                                      return std::sin(t * juce::MathConstants<float>::twoPi * 2.0f + 0.35f) * 0.45f;
                                  },
                                  false);
                break;
            }

            case 3: // Tape delay
                drawFeedbackLoop(g, bounds, mix);
                g.setColour(palette::kTextDim);
                g.setFont(fonts::label(8.5f));
                g.drawText("wow/flutter", bounds.removeFromBottom(14.0f), juce::Justification::centredRight);
                break;

            case 4: // Node delay
                drawNodeTree(g, bounds, 4);
                break;

            case 5: // Freq shift echo — descending spiral taps
            {
                juce::Path spiral;
                for (int i = 0; i <= 40; ++i)
                {
                    const float t = static_cast<float>(i) / 40.0f;
                    const float angle = t * juce::MathConstants<float>::twoPi * 2.5f;
                    const float radius = (1.0f - t * 0.65f) * bounds.getWidth() * 0.22f;
                    const float x = bounds.getCentreX() + std::cos(angle) * radius;
                    const float y = bounds.getCentreY() + std::sin(angle) * radius * 0.55f - t * bounds.getHeight() * 0.15f;
                    if (i == 0)
                        spiral.startNewSubPath(x, y);
                    else
                        spiral.lineTo(x, y);
                }
                strokeGlowPath(g, spiral, alpha, 1.8f, true);
                break;
            }

            case 6: // Fractal echo — branching tree
            {
                const float rootX = bounds.getX() + bounds.getWidth() * 0.12f;
                const float rootY = bounds.getCentreY();
                juce::Path tree;
                tree.startNewSubPath(rootX, rootY);
                tree.lineTo(bounds.getX() + bounds.getWidth() * 0.38f, bounds.getY() + bounds.getHeight() * 0.28f);
                tree.lineTo(bounds.getX() + bounds.getWidth() * 0.55f, bounds.getCentreY());
                tree.lineTo(bounds.getX() + bounds.getWidth() * 0.72f, bounds.getY() + bounds.getHeight() * 0.68f);
                tree.lineTo(bounds.getRight() - bounds.getWidth() * 0.08f, bounds.getCentreY());
                strokeGlowPath(g, tree, alpha, 1.8f, true);
                break;
            }

            case 7: // Reverb — FDN ring + early taps
            {
                const float cx = bounds.getCentreX();
                const float cy = bounds.getCentreY();
                const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.28f;
                g.setColour(palette::kAccent.withAlpha(alpha * 0.15f));
                g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);

                for (int i = 0; i < 8; ++i)
                {
                    const float angle = static_cast<float>(i) / 8.0f * juce::MathConstants<float>::twoPi;
                    const float x = cx + std::cos(angle) * radius;
                    const float y = cy + std::sin(angle) * radius;
                    g.setColour(palette::kAccent.withAlpha(alpha));
                    g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
                }

                juce::Path early;
                for (int i = 0; i < 6; ++i)
                {
                    const float t = static_cast<float>(i) / 5.0f;
                    const float x = bounds.getX() + bounds.getWidth() * (0.12f + t * 0.35f);
                    const float y = bounds.getBottom() - bounds.getHeight() * (0.15f + t * 0.25f);
                    if (i == 0)
                        early.startNewSubPath(x, y);
                    else
                        early.lineTo(x, y);
                }
                strokeGlowPath(g, early, alpha * 0.75f, 1.4f, false);
                break;
            }

            case 8: // EQ — 3-band response curve
                paintFlatWaveform(g, bounds, 64,
                                  [](float t)
                                  {
                                      const float low = 0.35f * std::exp(-std::pow((t - 0.18f) / 0.12f, 2.0f));
                                      const float mid = 0.55f * std::exp(-std::pow((t - 0.52f) / 0.08f, 2.0f));
                                      const float high = 0.25f * std::exp(-std::pow((t - 0.82f) / 0.14f, 2.0f));
                                      return low + mid - high;
                                  },
                                  true);
                break;

            case 9: // Compressor — soft-knee transfer + optional output transformer
                paintFlatWaveform(g, bounds, 48,
                                  [](float t)
                                  {
                                      const float x = t * 2.0f - 1.0f;
                                      if (x < -0.25f)
                                          return x * 0.55f;
                                      return -0.14f + (x + 0.25f) * 0.35f;
                                  },
                                  true);
                break;

            case 10: // Limiter — ceiling line
            {
                const float ceilingY = bounds.getY() + bounds.getHeight() * 0.28f;
                paintFlatWaveform(g, bounds, 48,
                                  [](float t)
                                  {
                                      const float x = t * 2.0f - 1.0f;
                                      return std::sin(x * juce::MathConstants<float>::pi * 1.5f) * 0.75f;
                                  },
                                  false);

                juce::Path ceiling;
                ceiling.startNewSubPath(bounds.getX(), ceilingY);
                ceiling.lineTo(bounds.getRight(), ceilingY);
                g.setColour(palette::kAccentWarm.withAlpha(alpha));
                g.strokePath(ceiling, juce::PathStrokeType(2.0f));
                break;
            }

            default:
                g.setColour(palette::kTextDim);
                g.drawText("?", bounds, juce::Justification::centred);
                break;
        }
    }

    void FxWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        paintEffectSchematic(g, bounds, effectType_, mix_);
    }

} // namespace pw8::plugin::ui::wireframe
