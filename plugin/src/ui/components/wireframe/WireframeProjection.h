#pragma once

#include <array>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../theme/ObsidianDraw.h"
#include "../../theme/ObsidianPalette.h"

// Shared pseudo-3D wireframe mesh projection and glow stroke used by every OSC
// engine preview (Wavetable stack, Classic cycle stack, FM dual-layer, etc.).
namespace pw8::plugin::ui::wireframe
{
    struct MeshLayout
    {
        float originXFrac = 0.42f;
        float originYFrac = 0.55f;
        float rowWidthFrac = 0.60f;
        float rowHeightFrac = 0.45f;
        float rowStepXFrac = 0.16f;
        float rowStepYFrac = 0.40f;
        float farShrink = 0.55f;
    };

    struct MeshDrawConfig
    {
        int meshRows = 15;
        int pointsPerRow = 48;
        int crossLineStride = 6;
        int liveRowIndex = 0;
        MeshLayout layout{};
    };

    inline juce::Rectangle<float> meshPaintBounds(juce::Rectangle<float> bounds, int captionHeight, int arrowWidth,
                                                 int margin)
    {
        bounds.removeFromBottom(static_cast<float>(captionHeight + margin));
        if (arrowWidth > 0)
        {
            bounds.removeFromLeft(static_cast<float>(arrowWidth + margin + 4));
            bounds.removeFromRight(static_cast<float>(arrowWidth + margin + 4));
        }
        return bounds;
    }

    inline void strokeGlowPath(juce::Graphics& g, const juce::Path& path, float alpha, float strokeWidth, bool live)
    {
        draw::strokeGlowPath(g, path, alpha, strokeWidth, live);
    }

    /// Paint a depth stack where row `r` samples `sampleAt(r, p)` in [-1, 1].
    template <typename SampleFn>
    void paintDepthMesh(juce::Graphics& g, juce::Rectangle<float> bounds, const MeshDrawConfig& config,
                         SampleFn&& sampleAt)
    {
        const int kMeshRows = config.meshRows;
        const int kPointsPerRow = config.pointsPerRow;
        const auto& layout = config.layout;

        const float originX = bounds.getX() + bounds.getWidth() * layout.originXFrac;
        const float originY = bounds.getY() + bounds.getHeight() * layout.originYFrac;
        const float rowWidth = bounds.getWidth() * layout.rowWidthFrac;
        const float rowHeight = bounds.getHeight() * layout.rowHeightFrac;
        const float stepX = bounds.getWidth() * layout.rowStepXFrac;
        const float stepY = bounds.getHeight() * layout.rowStepYFrac;

        using Row = std::array<juce::Point<float>, 64>;
        std::vector<Row> rows(static_cast<std::size_t>(kMeshRows));

        for (int r = 0; r < kMeshRows; ++r)
        {
            const float depthT = kMeshRows > 1 ? static_cast<float>(r) / static_cast<float>(kMeshRows - 1) : 0.0f;
            const float scale = 1.0f - (1.0f - layout.farShrink) * depthT;

            for (int p = 0; p < kPointsPerRow; ++p)
            {
                const float sampleValue = sampleAt(depthT, p);
                const float tp = kPointsPerRow > 1 ? static_cast<float>(p) / static_cast<float>(kPointsPerRow - 1)
                                                   : 0.0f;
                const float x = originX - rowWidth * 0.5f * scale + tp * rowWidth * scale + stepX * depthT;
                const float y = originY - stepY * depthT - sampleValue * rowHeight * 0.5f * scale;
                rows[static_cast<std::size_t>(r)][static_cast<std::size_t>(p)] = {x, y};
            }
        }

        for (int r = kMeshRows - 1; r >= 0; --r)
        {
            const auto& row = rows[static_cast<std::size_t>(r)];
            const float depthT = kMeshRows > 1 ? static_cast<float>(r) / static_cast<float>(kMeshRows - 1) : 0.0f;

            juce::Path linePath;
            float maxY = row[0].y;
            for (int p = 0; p < kPointsPerRow; ++p)
            {
                const auto& pt = row[static_cast<std::size_t>(p)];
                if (p == 0)
                    linePath.startNewSubPath(pt);
                else
                    linePath.lineTo(pt);
                maxY = juce::jmax(maxY, pt.y);
            }

            juce::Path silhouette(linePath);
            const float baselineY = maxY + 24.0f;
            silhouette.lineTo(row[static_cast<std::size_t>(kPointsPerRow - 1)].x, baselineY);
            silhouette.lineTo(row[0].x, baselineY);
            silhouette.closeSubPath();
            g.setColour(palette::kPanel);
            g.fillPath(silhouette);

            const bool isLive = r == config.liveRowIndex;
            const float alpha = isLive ? 1.0f : juce::jmap(depthT, 0.0f, 1.0f, 0.75f, 0.15f);
            const float strokeWidth = isLive ? 2.0f : juce::jmap(depthT, 0.0f, 1.0f, 1.4f, 0.6f);
            strokeGlowPath(g, linePath, alpha, strokeWidth, isLive);

            if (r < kMeshRows - 1)
            {
                const auto& fartherRow = rows[static_cast<std::size_t>(r + 1)];
                juce::Path crossPath;
                for (int p = 0; p < kPointsPerRow; p += config.crossLineStride)
                {
                    crossPath.startNewSubPath(row[static_cast<std::size_t>(p)]);
                    crossPath.lineTo(fartherRow[static_cast<std::size_t>(p)]);
                }
                g.setColour(palette::kBorderBright.withAlpha(alpha * 0.5f));
                g.strokePath(crossPath, juce::PathStrokeType(0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
            }
        }
    }

    /// Flat 2D waveform (one cycle) inside bounds.
    template <typename SampleFn>
    void paintFlatWaveform(juce::Graphics& g, juce::Rectangle<float> bounds, int points, SampleFn&& sampleAt,
                            bool liveGlow = true)
    {
        if (points < 2 || bounds.isEmpty())
            return;

        juce::Path path;
        for (int p = 0; p < points; ++p)
        {
            const float t = static_cast<float>(p) / static_cast<float>(points - 1);
            const float v = sampleAt(t);
            const float x = bounds.getX() + t * bounds.getWidth();
            const float y = bounds.getCentreY() - v * bounds.getHeight() * 0.42f;
            if (p == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        strokeGlowPath(g, path, liveGlow ? 1.0f : 0.65f, 2.0f, liveGlow);
    }

    /// Vertical bar chart in pseudo-3D (additive / resonator modes).
    template <typename HeightFn>
    void paintBarLandscape(juce::Graphics& g, juce::Rectangle<float> bounds, int barCount, HeightFn&& heightAt)
    {
        if (barCount <= 0 || bounds.isEmpty())
            return;

        const float barWidth = bounds.getWidth() / static_cast<float>(barCount + 1);
        for (int i = 0; i < barCount; ++i)
        {
            const float depthT = barCount > 1 ? static_cast<float>(i) / static_cast<float>(barCount - 1) : 0.0f;
            const float h = juce::jlimit(0.0f, 1.0f, heightAt(i));
            const float x = bounds.getX() + barWidth * (static_cast<float>(i) + 0.5f);
            const float towerH = h * bounds.getHeight() * 0.75f * (1.0f - depthT * 0.35f);
            const float yBase = bounds.getBottom() - bounds.getHeight() * 0.08f - depthT * bounds.getHeight() * 0.12f;
            const float alpha = juce::jmap(depthT, 0.0f, 1.0f, 1.0f, 0.35f);

            juce::Path bar;
            bar.startNewSubPath(x - barWidth * 0.25f, yBase);
            bar.lineTo(x - barWidth * 0.25f, yBase - towerH);
            bar.lineTo(x + barWidth * 0.25f, yBase - towerH);
            bar.lineTo(x + barWidth * 0.25f, yBase);
            bar.closeSubPath();

            g.setColour(palette::kAccent.withAlpha(alpha * 0.12f));
            g.fillPath(bar);

            juce::Path topEdge;
            topEdge.startNewSubPath(x - barWidth * 0.25f, yBase - towerH);
            topEdge.lineTo(x + barWidth * 0.25f, yBase - towerH);
            strokeGlowPath(g, topEdge, alpha, i == 0 ? 1.8f : 1.1f, i == 0);

            g.setColour((i == 0 ? palette::kAccent : palette::kBorderBright).withAlpha(alpha * 0.55f));
            g.strokePath(bar, juce::PathStrokeType(0.9f));
        }
    }

} // namespace pw8::plugin::ui::wireframe
