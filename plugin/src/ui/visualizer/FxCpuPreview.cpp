#include "FxCpuPreview.h"

#include "../components/wireframe/WireframeProjection.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "FxAnimationAtlas.h"
#include "PreviewDraw.h"
#include "VisualPreviewCache.h"

namespace pw8::plugin::ui::preview
{
    namespace
    {
        using wireframe::paintFlatWaveform;
        using wireframe::strokeGlowPath;

        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix,
                                      const char* suffix, float fallback)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + suffix))
                return raw->load();
            return fallback;
        }

        void paintPlotGrid(juce::Graphics& g, juce::Rectangle<float> plot)
        {
            g.setColour(palette::kBorder.withAlpha(0.28f));
            for (int i = 1; i < 5; ++i)
            {
                const float t = static_cast<float>(i) / 5.0f;
                g.drawVerticalLine(static_cast<int>(plot.getX() + plot.getWidth() * t), plot.getY(), plot.getBottom());
                g.drawHorizontalLine(static_cast<int>(plot.getY() + plot.getHeight() * t), plot.getX(), plot.getRight());
            }
        }

        void paintBypass(juce::Graphics& g, juce::Rectangle<float> plot, float mix)
        {
            paintPlotGrid(g, plot);
            juce::Path diag;
            diag.startNewSubPath(plot.getX(), plot.getBottom());
            diag.lineTo(plot.getRight(), plot.getY());
            g.setColour(palette::kTextDim.withAlpha(0.45f));
            g.strokePath(diag, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(9.0f));
            g.drawText("BYPASS · " + juce::String(mix, 2), plot, juce::Justification::centred);
        }

        void paintSaturation(juce::Graphics& g, juce::Rectangle<float> plot, float driveDb, float mix)
        {
            paintPlotGrid(g, plot);
            const float driveLinear = juce::Decibels::decibelsToGain(driveDb, -48.0f);
            const auto key = hashCombine(0x5A700001ULL, quantizeToKey(driveDb, 32.0f));
            const auto& poly = VisualPreviewCache::instance().getOrBuild(
                key, kDefaultPolylinePoints,
                [&](int n, PreviewPolyline& out)
                {
                    out.xNorm.clear();
                    out.yNorm.resize(static_cast<std::size_t>(n));
                    for (int i = 0; i < n; ++i)
                    {
                        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                        const float x = t * 2.0f - 1.0f;
                        out.yNorm[static_cast<std::size_t>(i)] = std::tanh(x * driveLinear * 0.55f);
                    }
                });
            PolylineDrawOptions opts;
            opts.alpha = juce::jlimit(0.45f, 1.0f, mix + 0.25f);
            paintPolylineCurve(g, plot, poly, opts);
        }

        void paintChorus(juce::Graphics& g, juce::Rectangle<float> plot, float rate, float depth, float mix)
        {
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001 * rate * 0.25);
            paintChorusAnimation(g, plot, rate, depth, mix, std::fmod(phase, 1.0f));
        }

        void paintDelay(juce::Graphics& g, juce::Rectangle<float> plot, float mix)
        {
            paintPlotGrid(g, plot);
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.002);
            juce::Path taps;
            for (int tap = 0; tap < 4; ++tap)
            {
                const float t0 = 0.12f + static_cast<float>(tap) * 0.22f;
                const float amp = 0.85f - static_cast<float>(tap) * 0.18f;
                juce::Path pulse;
                for (int i = 0; i <= 24; ++i)
                {
                    const float t = t0 + static_cast<float>(i) / 96.0f;
                    const float x = plot.getX() + t * plot.getWidth();
                    const float y = plot.getCentreY()
                                    - amp * std::exp(-static_cast<float>(i) * 0.08f)
                                          * std::sin(phase + static_cast<float>(tap)) * plot.getHeight() * 0.35f;
                    if (i == 0)
                        pulse.startNewSubPath(x, y);
                    else
                        pulse.lineTo(x, y);
                }
                taps.addPath(pulse);
            }
            strokeGlowPath(g, taps, juce::jlimit(0.45f, 1.0f, mix + 0.2f), 1.8f, true);
        }

        void paintReverb(juce::Graphics& g, juce::Rectangle<float> plot, float decaySec, float size, float damping,
                         float mix)
        {
            paintPlotGrid(g, plot);
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.0005);
            paintReverbAnimation(g, plot, decaySec, size, damping, mix, std::fmod(phase, 1.0f));
        }

        void paintEq(juce::Graphics& g, juce::Rectangle<float> plot, juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& prefix, float mix)
        {
            paintPlotGrid(g, plot);
            const float lowDb = readParam(apvts, prefix, "EqLowGainDb", 0.0f);
            const float midDb = readParam(apvts, prefix, "EqMidGainDb", 0.0f);
            const float highDb = readParam(apvts, prefix, "EqHighGainDb", 0.0f);
            const auto key = hashCombine(0xE0000003ULL, quantizeToKey(lowDb, 16.0f));
            const auto key2 = hashCombine(key, quantizeToKey(midDb, 16.0f));
            const auto key3 = hashCombine(key2, quantizeToKey(highDb, 16.0f));
            const auto& poly = VisualPreviewCache::instance().getOrBuild(
                key3, kDefaultPolylinePoints,
                [&](int n, PreviewPolyline& out)
                {
                    out.xNorm.clear();
                    out.yNorm.resize(static_cast<std::size_t>(n));
                    const float low = juce::Decibels::decibelsToGain(lowDb) - 1.0f;
                    const float mid = juce::Decibels::decibelsToGain(midDb) - 1.0f;
                    const float high = juce::Decibels::decibelsToGain(highDb) - 1.0f;
                    for (int i = 0; i < n; ++i)
                    {
                        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                        const float l = low * std::exp(-std::pow((t - 0.18f) / 0.10f, 2.0f));
                        const float m = mid * std::exp(-std::pow((t - 0.52f) / 0.08f, 2.0f));
                        const float h = high * std::exp(-std::pow((t - 0.82f) / 0.12f, 2.0f));
                        out.yNorm[static_cast<std::size_t>(i)] = (l + m + h) * 0.55f;
                    }
                });
            PolylineDrawOptions opts;
            opts.alpha = juce::jlimit(0.45f, 1.0f, mix + 0.15f);
            paintPolylineCurve(g, plot, poly, opts);
        }

        void paintCompressor(juce::Graphics& g, juce::Rectangle<float> plot, float thresholdDb, float ratio,
                             float mix)
        {
            paintPlotGrid(g, plot);
            const float threshNorm = juce::jmap(thresholdDb, -48.0f, 0.0f, 0.15f, 0.85f);
            const float ratioInv = 1.0f / juce::jmax(1.0f, ratio);
            const auto key = hashCombine(0xC0000004ULL, quantizeToKey(threshNorm, 32.0f));
            const auto key2 = hashCombine(key, quantizeToKey(ratio, 16.0f));
            const auto& poly = VisualPreviewCache::instance().getOrBuild(
                key2, 64,
                [&](int n, PreviewPolyline& out)
                {
                    out.xNorm.clear();
                    out.yNorm.resize(static_cast<std::size_t>(n));
                    for (int i = 0; i < n; ++i)
                    {
                        const float x = static_cast<float>(i) / static_cast<float>(n - 1);
                        float y = x;
                        if (x > threshNorm)
                            y = threshNorm + (x - threshNorm) * ratioInv;
                        out.yNorm[static_cast<std::size_t>(i)] = y * 2.0f - 1.0f;
                    }
                });
            PolylineDrawOptions opts;
            opts.alpha = juce::jlimit(0.45f, 1.0f, mix + 0.15f);
            paintPolylineCurve(g, plot, poly, opts);
        }

        void paintLimiter(juce::Graphics& g, juce::Rectangle<float> plot, float ceilingDb, float mix)
        {
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.003);
            paintFlatWaveform(g, plot, 56,
                              [phase](float t)
                              {
                                  const float x = t * 2.0f - 1.0f;
                                  return std::sin(x * juce::MathConstants<float>::pi * 1.5f + phase) * 0.75f;
                              },
                              true);
            const float ceilingNorm = juce::jmap(ceilingDb, -12.0f, 0.0f, 0.35f, 0.08f);
            const float ceilingY = plot.getY() + plot.getHeight() * ceilingNorm;
            g.setColour(palette::kAccentWarm.withAlpha(juce::jlimit(0.45f, 1.0f, mix + 0.35f)));
            g.drawHorizontalLine(static_cast<int>(ceilingY), plot.getX(), plot.getRight());
        }

        void paintVocoder(juce::Graphics& g, juce::Rectangle<float> plot, float mix)
        {
            const int bands = 16;
            const float barW = plot.getWidth() / static_cast<float>(bands + 1);
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001);
            for (int i = 0; i < bands; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(bands - 1);
                const float bump = std::exp(-std::pow((t - 0.45f) / 0.18f, 2.0f));
                const float motion = 0.08f * std::sin(phase * 3.2f + static_cast<float>(i) * 0.55f);
                const float h = plot.getHeight() * (0.12f + bump * 0.62f + mix * 0.12f + motion);
                const float x = plot.getX() + (static_cast<float>(i) + 0.5f) * barW;
                g.setColour(palette::kAccent.withAlpha(0.28f + bump * 0.55f));
                g.fillRect(x - barW * 0.32f, plot.getBottom() - h, barW * 0.64f, h);
            }
        }

        void paintClouds(juce::Graphics& g, juce::Rectangle<float> plot, float mix)
        {
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.0008);
            paintCloudsAnimation(g, plot, mix, 0xC10D5, std::fmod(phase, 1.0f));
        }

        void paintTapeDrift(juce::Graphics& g, juce::Rectangle<float> plot, float driftRate, float driftDepth,
                            float mix)
        {
            paintPlotGrid(g, plot);
            const float phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001 * driftRate * 0.2);
            paintTapeAnimation(g, plot, driftRate, driftDepth, mix, std::fmod(phase, 1.0f));
        }
    } // namespace

    void paintQuasarFieldPreview(juce::Graphics& g, juce::Rectangle<float> plot,
                                 juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix, float mix)
    {
        paintPlotGrid(g, plot);
        const auto centre = plot.getCentre();
        const float radius = juce::jmin(plot.getWidth(), plot.getHeight()) * 0.38f;

        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.2f);
        g.setColour(palette::kAccent.withAlpha(0.25f));
        g.fillEllipse(centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);

        const auto project = [&](float angleDeg, float dist) -> juce::Point<float>
        {
            const float rad = juce::degreesToRadians(angleDeg);
            return { centre.x + std::cos(rad) * radius * dist, centre.y + std::sin(rad) * radius * dist * 0.55f };
        };

        const float a1 = readParam(apvts, paramPrefix, "Qsr1AngleDeg", 30.0f);
        const float d1 = readParam(apvts, paramPrefix, "Qsr1Distance", 0.35f);
        const float a2 = readParam(apvts, paramPrefix, "Qsr2AngleDeg", 330.0f);
        const float d2 = readParam(apvts, paramPrefix, "Qsr2Distance", 0.35f);

        const auto p1 = project(a1, d1);
        const auto p2 = project(a2, d2);
        draw::fillGlowDot(g, p1, 8.0f, juce::Colour(0xff00c8ff), 0.95f, 5);
        draw::fillGlowDot(g, p2, 8.0f, juce::Colour(0xffe040fb), 0.95f, 5);

        g.setColour(juce::Colour(0xff00c8ff).withAlpha(0.35f * mix));
        g.drawLine(centre.x, centre.y, p1.x, p1.y, 1.0f);
        g.setColour(juce::Colour(0xffe040fb).withAlpha(0.35f * mix));
        g.drawLine(centre.x, centre.y, p2.x, p2.y, 1.0f);
    }

    void paintFxWireframePreview(juce::Graphics& g, juce::Rectangle<float> plot,
                                 juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                                 int effectType, float mix)
    {
        if (plot.isEmpty())
            return;

        g.setColour(palette::kBackgroundBottom.withAlpha(0.55f));
        g.fillRoundedRectangle(plot, 4.0f);

        switch (effectType)
        {
            case 0: paintBypass(g, plot, mix); break;
            case 1:
                paintSaturation(g, plot, readParam(apvts, paramPrefix, "SaturationDrive", 6.0f), mix);
                break;
            case 2:
                paintChorus(g, plot, readParam(apvts, paramPrefix, "ChorusRate", 0.5f),
                            readParam(apvts, paramPrefix, "ChorusDepth", 4.0f), mix);
                break;
            case 3:
                paintTapeDrift(g, plot, readParam(apvts, paramPrefix, "TapeDriftRate", 0.3f),
                               readParam(apvts, paramPrefix, "TapeDriftDepth", 1.5f), mix);
                break;
            case 4:
            case 5:
            case 6: paintDelay(g, plot, mix); break;
            case 7:
                paintReverb(g, plot, readParam(apvts, paramPrefix, "ReverbDecaySeconds", 2.0f),
                            readParam(apvts, paramPrefix, "ReverbSize", 1.0f),
                            readParam(apvts, paramPrefix, "ReverbHighRatio", 0.6f), mix);
                break;
            case 8: paintEq(g, plot, apvts, paramPrefix, mix); break;
            case 9:
                paintCompressor(g, plot, readParam(apvts, paramPrefix, "CompThresholdDb", -18.0f),
                                readParam(apvts, paramPrefix, "CompRatio", 4.0f), mix);
                break;
            case 10:
                paintLimiter(g, plot, readParam(apvts, paramPrefix, "LimiterCeilingDb", -0.3f), mix);
                break;
            case 11: paintVocoder(g, plot, mix); break;
            case 12: paintClouds(g, plot, mix); break;
            case 13: paintQuasarFieldPreview(g, plot, apvts, paramPrefix, mix); break;
            default: paintBypass(g, plot, mix); break;
        }
    }

} // namespace pw8::plugin::ui::preview
