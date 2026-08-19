#include "FxAnimationAtlas.h"

#include "../components/wireframe/WireframeProjection.h"
#include "../theme/ObsidianPalette.h"
#include "FxEmbeddedAtlases.h"
#include "VisualPreviewCache.h"
#include "VisualTheme.h"

namespace pw8::plugin::ui::preview
{
    namespace
    {
        using wireframe::paintFlatWaveform;
        using wireframe::strokeGlowPath;

        constexpr int kDefaultFrameCount = 24;

        [[nodiscard]] float framePhase01(int frameIndex, int frameCount) noexcept
        {
            return frameCount > 0 ? static_cast<float>(frameIndex) / static_cast<float>(frameCount) : 0.0f;
        }

        void blitFrame(juce::Image& atlas, const juce::Image& frame, int destX, int destY)
        {
            juce::Graphics ag(atlas);
            ag.drawImageAt(frame, destX, destY);
        }
    } // namespace

    uint64_t chorusAnimKey(float rateHz, float depthMs, float mix) noexcept
    {
        uint64_t h = hashCombine(0xC0A00001ULL, quantizeToKey(rateHz, 32.0f));
        h = hashCombine(h, quantizeToKey(depthMs, 32.0f));
        h = hashCombine(h, quantizeToKey(mix, 32.0f));
        return h;
    }

    uint64_t tapeAnimKey(float rate, float depth, float mix) noexcept
    {
        uint64_t h = hashCombine(0x7A900001ULL, quantizeToKey(rate, 32.0f));
        h = hashCombine(h, quantizeToKey(depth, 32.0f));
        h = hashCombine(h, quantizeToKey(mix, 32.0f));
        return h;
    }

    uint64_t reverbAnimKey(float decaySec, float size, float damping, float mix) noexcept
    {
        uint64_t h = hashCombine(0x2E100001ULL, quantizeToKey(decaySec, 32.0f));
        h = hashCombine(h, quantizeToKey(size, 32.0f));
        h = hashCombine(h, quantizeToKey(damping, 32.0f));
        h = hashCombine(h, quantizeToKey(mix, 32.0f));
        return h;
    }

    uint64_t cloudsAnimKey(float mix, int seed) noexcept
    {
        uint64_t h = hashCombine(0xC10D0001ULL, quantizeToKey(mix, 32.0f));
        h = hashCombine(h, static_cast<uint64_t>(seed));
        return h;
    }

    void paintChorusAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount,
                                float rateHz, float depthMs, float mix)
    {
        juce::ignoreUnused(mix);
        const float depthNorm = juce::jlimit(0.0f, 1.0f, depthMs / 12.0f);
        const float phase = framePhase01(frameIndex, frameCount) * juce::MathConstants<float>::twoPi
                            * juce::jlimit(0.2f, 4.0f, rateHz);
        const auto left = frame.removeFromLeft(frame.getWidth() * 0.48f).reduced(2.0f, 4.0f);
        const auto right = frame.reduced(2.0f, 4.0f);
        paintFlatWaveform(g, left, 32,
                          [depthNorm, phase](float t)
                          {
                              return std::sin(t * juce::MathConstants<float>::twoPi * 2.0f + phase)
                                   * (0.35f + depthNorm * 0.35f);
                          },
                          true);
        paintFlatWaveform(g, right, 32,
                          [depthNorm, phase](float t)
                          {
                              return std::sin(t * juce::MathConstants<float>::twoPi * 2.0f + phase + 0.35f)
                                   * (0.30f + depthNorm * 0.3f);
                          },
                          true);
    }

    void paintTapeAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount, float rate,
                              float depth, float mix)
    {
        const float wow = juce::jlimit(0.0f, 1.0f, depth / 20.0f);
        const float t0 = framePhase01(frameIndex, frameCount) * juce::MathConstants<float>::twoPi
                         * juce::jlimit(0.05f, 10.0f, rate);
        juce::Path drift;
        for (int i = 0; i <= 48; ++i)
        {
            const float t = static_cast<float>(i) / 48.0f;
            const float x = frame.getX() + t * frame.getWidth();
            const float wobble = std::sin(t * juce::MathConstants<float>::twoPi * 3.0f + t0) * wow * 0.18f;
            const float y = frame.getCentreY() + wobble * frame.getHeight();
            if (i == 0)
                drift.startNewSubPath(x, y);
            else
                drift.lineTo(x, y);
        }
        strokeGlowPath(g, drift, juce::jlimit(0.45f, 1.0f, mix + 0.2f), 2.0f, true);
    }

    void paintReverbAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount,
                                float decaySec, float size, float damping, float mix)
    {
        const float decayNorm = juce::jlimit(0.05f, 20.0f, decaySec) / 20.0f;
        const float damp = juce::jlimit(0.0f, 1.0f, damping);
        const float sweep = framePhase01(frameIndex, frameCount);
        juce::Path envelope;
        for (int i = 0; i <= 64; ++i)
        {
            const float t = static_cast<float>(i) / 64.0f;
            const float amp = std::exp(-t * (2.5f + (1.0f - decayNorm) * 4.0f + damp * 2.0f));
            const float shimmer = 1.0f + 0.08f * std::sin(sweep * juce::MathConstants<float>::twoPi + t * 8.0f);
            const float yNorm = amp * shimmer * (0.7f + size * 0.3f);
            const float x = frame.getX() + t * frame.getWidth();
            const float y = frame.getBottom() - yNorm * frame.getHeight() * 0.88f;
            if (i == 0)
                envelope.startNewSubPath(x, y);
            else
                envelope.lineTo(x, y);
        }
        strokeGlowPath(g, envelope, juce::jlimit(0.45f, 1.0f, mix + 0.2f), 2.0f, true);
    }

    void paintCloudsAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount, float mix,
                                int seed)
    {
        juce::Random rng(static_cast<int>(seed) ^ frameIndex * 7919);
        const float drift = framePhase01(frameIndex, frameCount);
        for (int i = 0; i < 28; ++i)
        {
            const float x = frame.getX() + std::fmod(rng.nextFloat() + drift * 0.35f, 1.0f) * frame.getWidth();
            const float y = frame.getY() + rng.nextFloat() * frame.getHeight();
            const float r = 1.2f + rng.nextFloat() * 2.2f;
            g.setColour((i % 3 == 0 ? palette::kAccent : palette::kAccentWarm).withAlpha(0.3f + rng.nextFloat() * 0.45f));
            g.fillEllipse(x - r, y - r, r * 2.0f, r * 2.0f);
        }
        juce::Path stream;
        const float xBias = (drift - 0.5f) * frame.getWidth() * 0.2f;
        stream.startNewSubPath(frame.getX() + 8.0f + xBias, frame.getCentreY());
        stream.quadraticTo(frame.getCentreX() + xBias, frame.getY() + frame.getHeight() * 0.25f, frame.getRight() - 8.0f,
                           frame.getCentreY());
        strokeGlowPath(g, stream, juce::jlimit(0.4f, 1.0f, mix + 0.15f), 1.4f, true);
    }

    FxAnimationAtlas& FxAnimationAtlas::instance() noexcept
    {
        static FxAnimationAtlas atlas;
        return atlas;
    }

    void FxAnimationAtlas::clear() noexcept
    {
        const juce::ScopedLock sl(lock_);
        entries_.clear();
        themeKey_ = visualCacheThemeKey();
    }

    const juce::Image& FxAnimationAtlas::getOrBake(
        FxAnimKind kind, uint64_t paramKey, int frameWidth, int frameHeight, int frameCount,
        const std::function<void(juce::Graphics&, juce::Rectangle<float>, int, int)>& bakeFrame)
    {
        const juce::ScopedLock sl(lock_);
        const auto currentTheme = visualCacheThemeKey();
        if (themeKey_ != 0 && themeKey_ != currentTheme)
            entries_.clear();
        themeKey_ = currentTheme;

        const uint64_t fullKey = withVisualThemeKey(hashCombine(static_cast<uint64_t>(kind), paramKey));

        for (const auto& entry : entries_)
        {
            if (entry.key == fullKey && entry.frameWidth == frameWidth && entry.frameHeight == frameHeight
                && entry.frameCount == frameCount && entry.atlas.isValid())
                return entry.atlas;
        }

        const auto& embedded = FxEmbeddedAtlases::instance();
        if (embedded.hasAtlas(kind) && paramKey == embedded.defaultParamKey(kind))
        {
            const auto& src = embedded.atlasFor(kind);
            if (src.isValid())
            {
                if (entries_.size() > 48)
                    entries_.clear();

                const int atlasW = frameWidth * frameCount;
                juce::Image atlas(juce::Image::ARGB, juce::jmax(1, atlasW), juce::jmax(1, frameHeight), true);
                juce::Graphics ag(atlas);
                for (int f = 0; f < frameCount; ++f)
                {
                    ag.drawImage(src, f * frameWidth, 0, frameWidth, frameHeight, f * FxEmbeddedAtlases::kFrameWidth,
                                 0, FxEmbeddedAtlases::kFrameWidth, FxEmbeddedAtlases::kFrameHeight);
                }

                Entry entry;
                entry.key = fullKey;
                entry.frameWidth = frameWidth;
                entry.frameHeight = frameHeight;
                entry.frameCount = frameCount;
                entry.atlas = std::move(atlas);
                entries_.push_back(std::move(entry));
                return entries_.back().atlas;
            }
        }

        if (entries_.size() > 48)
            entries_.clear();

        const int atlasW = frameWidth * frameCount;
        juce::Image atlas(juce::Image::ARGB, juce::jmax(1, atlasW), juce::jmax(1, frameHeight), true);

        for (int f = 0; f < frameCount; ++f)
        {
            juce::Image frameImg(juce::Image::ARGB, frameWidth, frameHeight, true);
            juce::Graphics fg(frameImg);
            const auto frameBounds =
                juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(frameWidth), static_cast<float>(frameHeight));
            bakeFrame(fg, frameBounds, f, frameCount);
            blitFrame(atlas, frameImg, f * frameWidth, 0);
        }

        Entry entry;
        entry.key = fullKey;
        entry.frameWidth = frameWidth;
        entry.frameHeight = frameHeight;
        entry.frameCount = frameCount;
        entry.atlas = std::move(atlas);
        entries_.push_back(std::move(entry));
        return entries_.back().atlas;
    }

    namespace
    {
        void paintAtlasFrame(juce::Graphics& g, juce::Rectangle<float> dest, const juce::Image& atlas, int frameWidth,
                              int frameHeight, int frameCount, int frameIndex, float alpha)
        {
            const int idx = juce::jlimit(0, frameCount - 1, frameIndex);
            g.setOpacity(alpha);
            g.drawImage(atlas, dest.getX(), dest.getY(), dest.getWidth(), dest.getHeight(), idx * frameWidth, 0,
                        frameWidth, frameHeight);
            g.setOpacity(1.0f);
        }

        [[nodiscard]] int frameIndexForPhase(float phase01, int frameCount) noexcept
        {
            return juce::jlimit(0, frameCount - 1,
                                static_cast<int>(phase01 * static_cast<float>(frameCount)) % juce::jmax(1, frameCount));
        }
    } // namespace

    void paintChorusAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float rateHz, float depthMs, float mix,
                               float phase01)
    {
        const int fw = juce::jmax(64, static_cast<int>(dest.getWidth()));
        const int fh = juce::jmax(32, static_cast<int>(dest.getHeight()));
        const auto& atlas = FxAnimationAtlas::instance().getOrBake(
            FxAnimKind::Chorus, chorusAnimKey(rateHz, depthMs, mix), fw, fh, kDefaultFrameCount,
            [=](juce::Graphics& fg, juce::Rectangle<float> fr, int fi, int fc)
            { paintChorusAtlasFrame(fg, fr, fi, fc, rateHz, depthMs, mix); });
        paintAtlasFrame(g, dest, atlas, fw, fh, kDefaultFrameCount, frameIndexForPhase(phase01, kDefaultFrameCount),
                        mix);
    }

    void paintTapeAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float rate, float depth, float mix,
                             float phase01)
    {
        const int fw = juce::jmax(64, static_cast<int>(dest.getWidth()));
        const int fh = juce::jmax(32, static_cast<int>(dest.getHeight()));
        const auto& atlas = FxAnimationAtlas::instance().getOrBake(
            FxAnimKind::TapeDrift, tapeAnimKey(rate, depth, mix), fw, fh, kDefaultFrameCount,
            [=](juce::Graphics& fg, juce::Rectangle<float> fr, int fi, int fc)
            { paintTapeAtlasFrame(fg, fr, fi, fc, rate, depth, mix); });
        paintAtlasFrame(g, dest, atlas, fw, fh, kDefaultFrameCount, frameIndexForPhase(phase01, kDefaultFrameCount),
                        mix);
    }

    void paintReverbAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float decaySec, float size, float damping,
                               float mix, float phase01)
    {
        const int fw = juce::jmax(64, static_cast<int>(dest.getWidth()));
        const int fh = juce::jmax(32, static_cast<int>(dest.getHeight()));
        const auto& atlas = FxAnimationAtlas::instance().getOrBake(
            FxAnimKind::ReverbDecay, reverbAnimKey(decaySec, size, damping, mix), fw, fh, kDefaultFrameCount,
            [=](juce::Graphics& fg, juce::Rectangle<float> fr, int fi, int fc)
            { paintReverbAtlasFrame(fg, fr, fi, fc, decaySec, size, damping, mix); });
        paintAtlasFrame(g, dest, atlas, fw, fh, kDefaultFrameCount, frameIndexForPhase(phase01, kDefaultFrameCount),
                        mix);
    }

    void paintCloudsAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float mix, int seed, float phase01)
    {
        const int fw = juce::jmax(64, static_cast<int>(dest.getWidth()));
        const int fh = juce::jmax(32, static_cast<int>(dest.getHeight()));
        const auto& atlas = FxAnimationAtlas::instance().getOrBake(
            FxAnimKind::Clouds, cloudsAnimKey(mix, seed), fw, fh, kDefaultFrameCount,
            [=](juce::Graphics& fg, juce::Rectangle<float> fr, int fi, int fc)
            { paintCloudsAtlasFrame(fg, fr, fi, fc, mix, seed); });
        paintAtlasFrame(g, dest, atlas, fw, fh, kDefaultFrameCount, frameIndexForPhase(phase01, kDefaultFrameCount),
                        mix);
    }

} // namespace pw8::plugin::ui::preview
