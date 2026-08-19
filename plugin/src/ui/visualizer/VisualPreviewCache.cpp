#include "VisualPreviewCache.h"

#include "../components/wireframe/EnvelopeCurveMath.h"
#include "../components/wireframe/EnvelopePathBuilder.h"
#include "VisualTheme.h"

namespace pw8::plugin::ui::preview
{
    uint64_t filterPreviewKey(int mode, float cutoffNorm, float resonance) noexcept
    {
        uint64_t h = 0xF11700001ULL;
        h = hashCombine(h, static_cast<uint64_t>(mode));
        h = hashCombine(h, quantizeToKey(cutoffNorm, 128.0f));
        h = hashCombine(h, quantizeToKey(resonance, 64.0f));
        return h;
    }

    uint64_t lfoPreviewKey(int waveform, int cycles, float phaseOffset) noexcept
    {
        uint64_t h = 0x4c464f0002ULL;
        h = hashCombine(h, static_cast<uint64_t>(waveform));
        h = hashCombine(h, static_cast<uint64_t>(cycles));
        h = hashCombine(h, quantizeToKey(phaseOffset, 64.0f));
        return h;
    }

    uint64_t envelopePreviewKey(float delay, float attack, float hold, float decay, float sustain, float release,
                                 float curveShape) noexcept
    {
        uint64_t h = 0xE00000003ULL;
        h = hashCombine(h, quantizeToKey(delay, 32.0f));
        h = hashCombine(h, quantizeToKey(attack, 64.0f));
        h = hashCombine(h, quantizeToKey(hold, 32.0f));
        h = hashCombine(h, quantizeToKey(decay, 64.0f));
        h = hashCombine(h, quantizeToKey(sustain, 64.0f));
        h = hashCombine(h, quantizeToKey(release, 64.0f));
        h = hashCombine(h, quantizeToKey(curveShape, 32.0f));
        return h;
    }

    VisualPreviewCache& VisualPreviewCache::instance() noexcept
    {
        static VisualPreviewCache cache;
        return cache;
    }

    void VisualPreviewCache::clear() noexcept
    {
        const std::lock_guard lock(mutex_);
        entries_.clear();
        themeKey_ = visualCacheThemeKey();
    }

    void VisualPreviewCache::invalidateIfThemeChanged() noexcept
    {
        const auto current = visualCacheThemeKey();
        if (themeKey_ != 0 && themeKey_ != current)
            entries_.clear();
        themeKey_ = current;
    }

    const PreviewPolyline& VisualPreviewCache::getOrBuild(
        uint64_t key, int pointCount, const std::function<void(int, PreviewPolyline&)>& builder)
    {
        const std::lock_guard lock(mutex_);
        invalidateIfThemeChanged();
        const uint64_t themedKey = withVisualThemeKey(key);
        if (const auto it = entries_.find(themedKey); it != entries_.end())
            return it->second;

        if (entries_.size() >= kMaxCacheEntries)
            entries_.clear();

        PreviewPolyline polyline;
        builder(pointCount, polyline);
        const auto [inserted, _] = entries_.emplace(themedKey, std::move(polyline));
        return inserted->second;
    }

    void buildEnvelopePolyline(const wireframe::EnvelopePreviewParams& params, int pointCount, PreviewPolyline& out)
    {
        out.xNorm.clear();
        out.yNorm.clear();
        out.sustainEndNorm = 0.0f;

        if (pointCount < 2)
            return;

        juce::Path outline;
        juce::Path fillPath;
        float sustainEndX = 0.0f;
        const juce::Rectangle<float> unitBounds(0.0f, 0.0f, 1.0f, 1.0f);
        wireframe::buildEnvelopePath(params, unitBounds, outline, fillPath, sustainEndX);

        const auto layout = wireframe::computeEnvelopeLayout(params, unitBounds);
        out.sustainEndNorm =
            layout.totalDisplaySeconds > 0.0f ? sustainEndX / layout.totalDisplaySeconds : 0.0f;

        const float length = outline.getLength();
        if (length <= 0.0f)
            return;

        out.xNorm.resize(static_cast<std::size_t>(pointCount));
        out.yNorm.resize(static_cast<std::size_t>(pointCount));
        for (int i = 0; i < pointCount; ++i)
        {
            const float d = (static_cast<float>(i) / static_cast<float>(pointCount - 1)) * length;
            const juce::Point<float> pt = outline.getPointAlongPath(d);
            out.xNorm[static_cast<std::size_t>(i)] = juce::jlimit(0.0f, 1.0f, pt.x);
            out.yNorm[static_cast<std::size_t>(i)] = juce::jlimit(0.0f, 1.0f, layout.levelFromY(pt.y));
        }
    }

} // namespace pw8::plugin::ui::preview
