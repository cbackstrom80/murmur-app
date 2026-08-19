#include "WavetableMeshPaint.h"

#include <cmath>
#include <limits>
#include <mutex>
#include <unordered_map>

#include "../../theme/ObsidianPalette.h"
#include "../../visualizer/PreviewDraw.h"
#include "../../visualizer/VisualPreviewCache.h"
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

        [[nodiscard]] uint64_t wavetableMeshSampleKey(const oscillator::WavetableTable* table,
                                                       const oscillator::WtWarpParams& warp, int meshRows,
                                                       int pointsPerRow) noexcept
        {
            uint64_t h = reinterpret_cast<uint64_t>(table);
            h = preview::hashCombine(h, static_cast<uint64_t>(table != nullptr ? table->numFrames : 0));
            h = preview::hashCombine(h, static_cast<uint64_t>(table != nullptr ? table->samplesPerFrame : 0));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.bend, 32.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.asymmetry, 32.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.syncAmount, 32.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.formantShift, 32.0f));
            h = preview::hashCombine(h, static_cast<uint64_t>(meshRows));
            h = preview::hashCombine(h, static_cast<uint64_t>(pointsPerRow));
            return h;
        }

        [[nodiscard]] uint64_t wavetableFrameKey(const oscillator::WavetableTable* table, float framePos01,
                                                  const oscillator::WtWarpParams& warp, int points) noexcept
        {
            uint64_t h = reinterpret_cast<uint64_t>(table);
            h = preview::hashCombine(h, preview::quantizeToKey(framePos01, 128.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.bend, 32.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.asymmetry, 32.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.syncAmount, 32.0f));
            h = preview::hashCombine(h, preview::quantizeToKey(warp.formantShift, 32.0f));
            h = preview::hashCombine(h, static_cast<uint64_t>(points));
            return h;
        }

        struct MeshSampleCacheEntry
        {
            int meshRows = 0;
            int pointsPerRow = 0;
            std::vector<std::vector<float>> samples;
        };

        std::mutex meshSampleCacheMutex;
        std::unordered_map<uint64_t, MeshSampleCacheEntry> meshSampleCache;
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

        const auto cacheKey =
            wavetableMeshSampleKey(table, warpParams, options.meshRows, options.pointsPerRow);
        MeshSampleCacheEntry* cached = nullptr;
        {
            const std::lock_guard lock(meshSampleCacheMutex);
            auto& entry = meshSampleCache[cacheKey];
            if (entry.samples.empty())
            {
                entry.meshRows = options.meshRows;
                entry.pointsPerRow = options.pointsPerRow;
                entry.samples.resize(static_cast<std::size_t>(options.meshRows));
                for (int r = 0; r < options.meshRows; ++r)
                {
                    entry.samples[static_cast<std::size_t>(r)].resize(static_cast<std::size_t>(options.pointsPerRow));
                    const float depthT = options.meshRows > 1 ? static_cast<float>(r) / static_cast<float>(options.meshRows - 1)
                                                              : 0.0f;
                    for (int p = 0; p < options.pointsPerRow; ++p)
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

                        const std::size_t off0 =
                            static_cast<std::size_t>(f0) * static_cast<std::size_t>(samplesPerFrame);
                        const std::size_t off1 =
                            static_cast<std::size_t>(f1) * static_cast<std::size_t>(samplesPerFrame);
                        const float v0 = pw8::dsp::lerp(mip.samples[off0 + static_cast<std::size_t>(s0)],
                                                          mip.samples[off0 + static_cast<std::size_t>(s1)], sampleFrac);
                        const float v1 = pw8::dsp::lerp(mip.samples[off1 + static_cast<std::size_t>(s0)],
                                                          mip.samples[off1 + static_cast<std::size_t>(s1)], sampleFrac);
                        entry.samples[static_cast<std::size_t>(r)][static_cast<std::size_t>(p)] =
                            pw8::dsp::lerp(v0, v1, frameFrac);
                    }
                }
            }
            cached = &entry;
        }

        const auto sampleAt = [&](float depthT, int p) -> float
        {
            const int r = juce::jlimit(0, cached->meshRows - 1,
                                       static_cast<int>(depthT * static_cast<float>(cached->meshRows - 1) + 0.5f));
            return cached->samples[static_cast<std::size_t>(r)][static_cast<std::size_t>(p)];
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

        const int sampleCount = juce::jmax(8, points);
        const auto key = wavetableFrameKey(table, framePos01, warpParams, sampleCount);
        const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
            key, sampleCount,
            [&](int n, preview::PreviewPolyline& out)
            {
                out.xNorm.clear();
                out.yNorm.resize(static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i)
                {
                    const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                    out.yNorm[static_cast<std::size_t>(i)] = sampleWavetableAt(table, framePos01, t, warpParams);
                }
            });

        preview::PolylineDrawOptions opts;
        opts.liveGlow = liveGlow;
        opts.alpha = liveGlow ? 1.0f : 0.65f;
        preview::paintPolylineCurve(g, bounds, polyline, opts);
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
