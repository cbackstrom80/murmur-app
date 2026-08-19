#pragma once

#include <cmath>

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianPalette.h"
#include "AudioVisualizerBus.h"
#include "PreviewSurface.h"
#include "VisualPreviewCache.h"

namespace pw8::plugin::ui::preview
{

struct PolylineDrawOptions
{
    float alpha = 1.0f;
    float strokeWidth = 2.0f;
    bool liveGlow = true;
    bool fillUnder = true;
    float fillAlpha = 0.12f;
    float yScale = 0.42f;
};

/// Paint a cached polyline scaled into `bounds`. Uses 2× supersampling when plot is large enough.
inline void paintPolylineCurve(juce::Graphics& g, juce::Rectangle<float> bounds, const PreviewPolyline& polyline,
                               const PolylineDrawOptions& opts = {})
{
    if (polyline.yNorm.size() < 2 || bounds.isEmpty())
        return;

    const auto drawDirect = [&](juce::Graphics& gg, juce::Rectangle<float> plot)
    {
        const int n = static_cast<int>(polyline.yNorm.size());
        const bool uniformX = polyline.xNorm.empty() || polyline.xNorm.size() != polyline.yNorm.size();

        juce::Path stroke;
        juce::Path fill;
        const float midY = plot.getCentreY();
        const float halfH = plot.getHeight() * opts.yScale;

        for (int i = 0; i < n; ++i)
        {
            const float xNorm =
                uniformX ? static_cast<float>(i) / static_cast<float>(n - 1) : polyline.xNorm[static_cast<std::size_t>(i)];
            const float yVal = polyline.yNorm[static_cast<std::size_t>(i)];
            const float x = plot.getX() + xNorm * plot.getWidth();
            const float y = uniformX ? midY - yVal * halfH
                                     : plot.getBottom() - yVal * plot.getHeight() * 0.88f - plot.getHeight() * 0.06f;

            if (i == 0)
            {
                stroke.startNewSubPath(x, y);
                fill.startNewSubPath(x, plot.getBottom());
                fill.lineTo(x, y);
            }
            else
            {
                stroke.lineTo(x, y);
                fill.lineTo(x, y);
            }
        }

        if (opts.fillUnder)
        {
            fill.lineTo(stroke.getBounds().getRight(), plot.getBottom());
            fill.lineTo(plot.getX(), plot.getBottom());
            fill.closeSubPath();
            gg.setColour(palette::kAccent.withAlpha(opts.fillAlpha * opts.alpha));
            gg.fillPath(fill);
        }

        draw::strokeGlowPath(gg, stroke, opts.alpha, opts.strokeWidth, opts.liveGlow);
    };

    if (bounds.getWidth() >= 48.0f && bounds.getHeight() >= 28.0f)
    {
        const juce::Image hiRes = renderPlotHiRes(bounds, drawDirect, PreviewSurface::kSupersampleScale);
        if (hiRes.isValid())
        {
            g.drawImage(hiRes, bounds);
            return;
        }
    }

    drawDirect(g, bounds);
}

/// Back-compat alias kept for callers that want explicit naming.
inline void paintPolylineCurveHiRes(juce::Graphics& g, juce::Rectangle<float> bounds, const PreviewPolyline& polyline,
                                    const PolylineDrawOptions& opts = {})
{
    paintPolylineCurve(g, bounds, polyline, opts);
}

/// Min/max ribbon from the lock-free waveform ring (512 columns, one pair per audio block).
inline void paintWaveformFromBus(juce::Graphics& g, juce::Rectangle<float> plot,
                                 const murmur8::AudioVisualizerBus& bus, float alpha = 0.85f)
{
    if (plot.isEmpty())
        return;

    constexpr int kCols = murmur8::AudioVisualizerBus::waveformColumns;
    const auto& ring = bus.getWaveform();

    float peak = 0.0f;
    juce::Path ribbon;
    juce::Path centerLine;

    const float midY = plot.getCentreY();
    const float halfH = plot.getHeight() * 0.42f;

    for (int i = 0; i < kCols; ++i)
    {
        const float minV = ring.mins[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        const float maxV = ring.maxs[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        peak = juce::jmax(peak, std::abs(minV), std::abs(maxV));

        const float x = plot.getX() + (static_cast<float>(i) / static_cast<float>(kCols - 1)) * plot.getWidth();
        const float yTop = midY - juce::jlimit(-1.0f, 1.0f, maxV) * halfH;
        const float yBot = midY - juce::jlimit(-1.0f, 1.0f, minV) * halfH;
        const float yMid = (yTop + yBot) * 0.5f;

        if (i == 0)
        {
            ribbon.startNewSubPath(x, yTop);
            centerLine.startNewSubPath(x, yMid);
        }
        else
        {
            ribbon.lineTo(x, yTop);
            centerLine.lineTo(x, yMid);
        }
    }

    for (int i = kCols - 1; i >= 0; --i)
    {
        const float minV = ring.mins[static_cast<std::size_t>(i)].load(std::memory_order_relaxed);
        const float x = plot.getX() + (static_cast<float>(i) / static_cast<float>(kCols - 1)) * plot.getWidth();
        const float yBot = midY - juce::jlimit(-1.0f, 1.0f, minV) * halfH;
        ribbon.lineTo(x, yBot);
    }
    ribbon.closeSubPath();

    if (peak < 0.0005f)
        return;

    g.setColour(palette::kAccent.withAlpha(0.10f * alpha));
    g.fillPath(ribbon);
    draw::strokeGlowPath(g, centerLine, 0.55f * alpha, 1.8f, true);
    g.setColour(palette::kAccent.withAlpha(0.9f * alpha));
    g.strokePath(centerLine, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

/// Log-spaced FFT bars from AudioVisualizerBus (128 bins).
inline void paintSpectrumFromBus(juce::Graphics& g, juce::Rectangle<float> plot,
                                 const murmur8::AudioVisualizerBus& bus, float alpha = 0.9f)
{
    if (plot.isEmpty())
        return;

    std::array<float, murmur8::AudioVisualizerBus::fftBinCount> bins {};
    bus.readFFTInto(bins);

    constexpr int kBins = murmur8::AudioVisualizerBus::fftBinCount;
    const float barW = plot.getWidth() / static_cast<float>(kBins);

    juce::Path outline;
    for (int i = 0; i < kBins; ++i)
    {
        const float level = juce::jlimit(0.0f, 1.0f, bins[static_cast<std::size_t>(i)]);
        const float x = plot.getX() + static_cast<float>(i) * barW;
        const float h = level * (plot.getHeight() - 8.0f);
        g.setColour(palette::kAccent.withAlpha((0.25f + level * 0.55f) * alpha));
        g.fillRect(x + 1.0f, plot.getBottom() - 4.0f - h, juce::jmax(1.0f, barW - 2.0f), h);

        const float yTop = plot.getBottom() - 4.0f - h;
        if (i == 0)
            outline.startNewSubPath(x + barW * 0.5f, yTop);
        else
            outline.lineTo(x + barW * 0.5f, yTop);
    }

    if (!outline.isEmpty())
        draw::strokeGlowPath(g, outline, 0.35f * alpha, 1.2f, true);
}

} // namespace pw8::plugin::ui::preview
