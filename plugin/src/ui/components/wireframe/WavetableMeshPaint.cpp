#include "WavetableMeshPaint.h"

#include <cmath>
#include <limits>

#include "../../theme/ObsidianPalette.h"
#include "pw8/dsp/Math.hpp"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] int liveRowForPosition(float livePos01, int numFrames, int meshRows)
        {
            if (numFrames <= 1 || meshRows <= 1)
                return 0;

            const float liveFramePos = juce::jlimit(0.0f, 1.0f, livePos01) * static_cast<float>(numFrames - 1);
            int liveRowIndex = 0;
            float liveRowDist = std::numeric_limits<float>::max();
            for (int r = 0; r < meshRows; ++r)
            {
                const float depthT = static_cast<float>(r) / static_cast<float>(meshRows - 1);
                const float rowFramePos = depthT * static_cast<float>(numFrames - 1);
                if (const float dist = std::abs(rowFramePos - liveFramePos); dist < liveRowDist)
                {
                    liveRowDist = dist;
                    liveRowIndex = r;
                }
            }
            return liveRowIndex;
        }
    } // namespace

    WavetableMeshPaintOptions labHeroMeshOptions()
    {
        WavetableMeshPaintOptions opts;
        opts.meshRows = 15;
        opts.layout.originXFrac = 0.42f;
        opts.layout.originYFrac = 0.55f;
        opts.layout.rowWidthFrac = 0.60f;
        opts.layout.rowHeightFrac = 0.45f;
        opts.layout.rowStepXFrac = 0.16f;
        opts.layout.rowStepYFrac = 0.40f;
        opts.layout.farShrink = 0.55f;
        return opts;
    }

    WavetableMeshPaintOptions contextThumbMeshOptions()
    {
        WavetableMeshPaintOptions opts;
        opts.meshRows = 9;
        opts.pointsPerRow = 40;
        opts.crossLineStride = 8;
        opts.layout.originXFrac = 0.36f;
        opts.layout.originYFrac = 0.72f;
        opts.layout.rowWidthFrac = 0.72f;
        opts.layout.rowHeightFrac = 0.50f;
        opts.layout.rowStepXFrac = 0.12f;
        opts.layout.rowStepYFrac = 0.32f;
        opts.layout.farShrink = 0.62f;
        return opts;
    }

    void paintWavetableTableMesh(juce::Graphics& g, juce::Rectangle<float> bounds,
                                 const oscillator::WavetableTable* table,
                                 const oscillator::WtWarpParams& warpParams, float livePos01,
                                 const WavetableMeshPaintOptions& options)
    {
        if (table == nullptr || !table->isValid() || bounds.isEmpty())
            return;

        const auto& mip = table->mips.front();
        const int numFrames = table->numFrames;
        const int samplesPerFrame = table->samplesPerFrame;
        if (numFrames <= 0 || samplesPerFrame <= 1
            || mip.samples.size() < static_cast<std::size_t>(numFrames) * static_cast<std::size_t>(samplesPerFrame))
            return;

        const int liveRowIndex = liveRowForPosition(livePos01, numFrames, options.meshRows);

        const auto sampleAt = [&](float depthT, int p) -> float
        {
            const float framePos = depthT * static_cast<float>(numFrames - 1);
            const int f0 = static_cast<int>(framePos);
            const int f1 = juce::jmin(f0 + 1, numFrames - 1);
            const float frameFrac = framePos - static_cast<float>(f0);

            const float tp = options.pointsPerRow > 1
                                 ? static_cast<float>(p) / static_cast<float>(options.pointsPerRow - 1)
                                 : 0.0f;
            const float readPhase = oscillator::warpReadPhase(tp, warpParams);
            const float srcPos = readPhase * static_cast<float>(samplesPerFrame - 1);
            const int s0 = static_cast<int>(srcPos);
            const int s1 = juce::jmin(s0 + 1, samplesPerFrame - 1);
            const float sampleFrac = srcPos - static_cast<float>(s0);

            const std::size_t off0 = static_cast<std::size_t>(f0) * static_cast<std::size_t>(samplesPerFrame);
            const std::size_t off1 = static_cast<std::size_t>(f1) * static_cast<std::size_t>(samplesPerFrame);
            const float v0 = pw8::dsp::lerp(mip.samples[off0 + static_cast<std::size_t>(s0)],
                                             mip.samples[off0 + static_cast<std::size_t>(s1)], sampleFrac);
            const float v1 = pw8::dsp::lerp(mip.samples[off1 + static_cast<std::size_t>(s0)],
                                             mip.samples[off1 + static_cast<std::size_t>(s1)], sampleFrac);
            return pw8::dsp::lerp(v0, v1, frameFrac);
        };

        MeshDrawConfig config;
        config.meshRows = options.meshRows;
        config.pointsPerRow = options.pointsPerRow;
        config.crossLineStride = options.crossLineStride;
        config.liveRowIndex = liveRowIndex;
        config.layout = options.layout;
        paintDepthMesh(g, bounds, config, sampleAt);
    }

    float sampleWavetableAt(const oscillator::WavetableTable* table, float framePos01, float phase01,
                            const oscillator::WtWarpParams& warpParams)
    {
        if (table == nullptr || !table->isValid())
            return 0.0f;

        const auto& mip = table->mips.front();
        const int numFrames = table->numFrames;
        const int samplesPerFrame = table->samplesPerFrame;
        if (numFrames <= 0 || samplesPerFrame <= 1
            || mip.samples.size() < static_cast<std::size_t>(numFrames) * static_cast<std::size_t>(samplesPerFrame))
            return 0.0f;

        const float framePos = juce::jlimit(0.0f, 1.0f, framePos01) * static_cast<float>(numFrames - 1);
        const int f0 = static_cast<int>(framePos);
        const int f1 = juce::jmin(f0 + 1, numFrames - 1);
        const float frameFrac = framePos - static_cast<float>(f0);

        const float readPhase = oscillator::warpReadPhase(juce::jlimit(0.0f, 1.0f, phase01), warpParams);
        const float srcPos = readPhase * static_cast<float>(samplesPerFrame - 1);
        const int s0 = static_cast<int>(srcPos);
        const int s1 = juce::jmin(s0 + 1, samplesPerFrame - 1);
        const float sampleFrac = srcPos - static_cast<float>(s0);

        const std::size_t off0 = static_cast<std::size_t>(f0) * static_cast<std::size_t>(samplesPerFrame);
        const std::size_t off1 = static_cast<std::size_t>(f1) * static_cast<std::size_t>(samplesPerFrame);
        const float v0 = pw8::dsp::lerp(mip.samples[off0 + static_cast<std::size_t>(s0)],
                                         mip.samples[off0 + static_cast<std::size_t>(s1)], sampleFrac);
        const float v1 = pw8::dsp::lerp(mip.samples[off1 + static_cast<std::size_t>(s0)],
                                         mip.samples[off1 + static_cast<std::size_t>(s1)], sampleFrac);
        return pw8::dsp::lerp(v0, v1, frameFrac);
    }

    void paintWavetableFrameWaveform(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     const oscillator::WavetableTable* table, float framePos01,
                                     const oscillator::WtWarpParams& warpParams, bool liveGlow, int points)
    {
        if (table == nullptr || !table->isValid() || bounds.isEmpty())
            return;

        paintFlatWaveform(
            g, bounds, juce::jmax(8, points),
            [&](float t) { return sampleWavetableAt(table, framePos01, t, warpParams); }, liveGlow);
    }

    void computeWavetableHarmonicMagnitudes(const oscillator::WavetableTable* table, float framePos01,
                                            const oscillator::WtWarpParams& warpParams,
                                            std::array<float, 16>& magnitudesOut)
    {
        magnitudesOut.fill(0.0f);
        if (table == nullptr || !table->isValid())
            return;

        const int samplesPerFrame = table->samplesPerFrame;
        if (samplesPerFrame <= 1)
            return;

        constexpr int kAnalysisPoints = 256;
        std::array<float, kAnalysisPoints> frameSamples{};
        for (int i = 0; i < kAnalysisPoints; ++i)
        {
            const float phase = static_cast<float>(i) / static_cast<float>(kAnalysisPoints - 1);
            frameSamples[static_cast<std::size_t>(i)] = sampleWavetableAt(table, framePos01, phase, warpParams);
        }

        float peak = 0.0f;
        for (int h = 1; h <= 16; ++h)
        {
            float re = 0.0f;
            float im = 0.0f;
            for (int i = 0; i < kAnalysisPoints; ++i)
            {
                const float angle =
                    juce::MathConstants<float>::twoPi * static_cast<float>(h) * static_cast<float>(i)
                    / static_cast<float>(kAnalysisPoints);
                const float sample = frameSamples[static_cast<std::size_t>(i)];
                re += sample * std::cos(angle);
                im += sample * std::sin(angle);
            }
            const float mag = std::sqrt(re * re + im * im) / static_cast<float>(kAnalysisPoints);
            magnitudesOut[static_cast<std::size_t>(h - 1)] = mag;
            peak = juce::jmax(peak, mag);
        }

        if (peak > 1.0e-6f)
        {
            for (auto& m : magnitudesOut)
                m = juce::jlimit(0.0f, 1.0f, m / peak);
        }
    }

    void paintGranularGrainOverlay(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   const GranularOverlayParams& params)
    {
        if (bounds.isEmpty())
            return;

        const float pos = juce::jlimit(0.0f, 1.0f, params.wavetablePos);
        const float grainW =
            juce::jmap(params.grainSizeMs, 1.0f, 500.0f, bounds.getWidth() * 0.06f, bounds.getWidth() * 0.22f);

        for (int gIdx = 0; gIdx < 3; ++gIdx)
        {
            const float cx =
                bounds.getX()
                + bounds.getWidth() * juce::jlimit(0.05f, 0.95f, pos + static_cast<float>(gIdx - 1) * 0.12f);
            const float top = bounds.getY() + bounds.getHeight() * 0.15f;
            const float h = bounds.getHeight() * 0.55f;
            juce::Rectangle<float> grain(cx - grainW * 0.5f, top, grainW, h);
            g.setColour(palette::kAccentWarm.withAlpha(0.12f));
            g.fillRect(grain);
            g.setColour(palette::kAccentWarm.withAlpha(0.75f));
            g.drawRect(grain, 1.2f);
        }
    }

} // namespace pw8::plugin::ui::wireframe
