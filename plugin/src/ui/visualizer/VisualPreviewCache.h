#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <juce_graphics/juce_graphics.h>

#include "../components/wireframe/EnvelopeCurveMath.h"

namespace pw8::plugin::ui::preview
{

inline constexpr int kDefaultPolylinePoints = 128;
inline constexpr int kMaxCacheEntries = 256;

struct PreviewPolyline
{
    /// Uniform-x curves: empty `xNorm`, `yNorm.size()` samples.
    /// Envelope curves: paired normalized coordinates in unit square.
    std::vector<float> xNorm;
    std::vector<float> yNorm;
    float sustainEndNorm = 0.0f;
};

[[nodiscard]] inline uint64_t hashCombine(uint64_t seed, uint64_t value) noexcept
{
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

[[nodiscard]] inline uint64_t quantizeToKey(float value, float steps) noexcept
{
    const float q = steps > 1.0f ? std::round(value * steps) / steps : value;
    return static_cast<uint64_t>(std::lround(q * 100000.0f));
}

[[nodiscard]] uint64_t filterPreviewKey(int mode, float cutoffNorm, float resonance) noexcept;
[[nodiscard]] uint64_t lfoPreviewKey(int waveform, int cycles, float phaseOffset) noexcept;
[[nodiscard]] uint64_t envelopePreviewKey(float delay, float attack, float hold, float decay, float sustain,
                                          float release, float curveShape) noexcept;

void buildEnvelopePolyline(const wireframe::EnvelopePreviewParams& params, int pointCount, PreviewPolyline& out);

/// Control-thread polyline cache for static/parametric previews (filter, LFO, ADSR, etc.).
class VisualPreviewCache
{
public:
    [[nodiscard]] static VisualPreviewCache& instance() noexcept;

    [[nodiscard]] const PreviewPolyline& getOrBuild(
        uint64_t key, int pointCount,
        const std::function<void(int pointCount, PreviewPolyline& out)>& builder);

    void clear() noexcept;
    void invalidateIfThemeChanged() noexcept;

private:
    VisualPreviewCache() = default;

    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, PreviewPolyline> entries_;
    uint64_t themeKey_ = 0;
};

} // namespace pw8::plugin::ui::preview
