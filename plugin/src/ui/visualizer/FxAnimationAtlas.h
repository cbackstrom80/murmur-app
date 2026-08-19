#pragma once

#include <cstdint>
#include <functional>

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::preview
{

enum class FxAnimKind : int
{
    Chorus = 0,
    TapeDrift,
    ReverbDecay,
    Clouds,
    FractalStream,
};

/// Runtime-baked horizontal sprite atlases for animated FX hero / wireframe previews.
class FxAnimationAtlas
{
public:
    [[nodiscard]] static FxAnimationAtlas& instance() noexcept;

    [[nodiscard]] const juce::Image& getOrBake(FxAnimKind kind, uint64_t paramKey, int frameWidth, int frameHeight,
                                                int frameCount,
                                                const std::function<void(juce::Graphics&, juce::Rectangle<float>, int,
                                                                         int)>& bakeFrame);

    void clear() noexcept;

private:
    FxAnimationAtlas() = default;

    struct Entry
    {
        uint64_t key = 0;
        int frameWidth = 0;
        int frameHeight = 0;
        int frameCount = 0;
        juce::Image atlas;
    };

    mutable juce::CriticalSection lock_;
    std::vector<Entry> entries_;
    uint64_t themeKey_ = 0;
};

uint64_t chorusAnimKey(float rateHz, float depthMs, float mix) noexcept;
uint64_t tapeAnimKey(float rate, float depth, float mix) noexcept;
uint64_t reverbAnimKey(float decaySec, float size, float damping, float mix) noexcept;
uint64_t cloudsAnimKey(float mix, int seed) noexcept;

void paintChorusAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount,
                             float rateHz, float depthMs, float mix);
void paintTapeAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount, float rate,
                          float depth, float mix);
void paintReverbAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount,
                            float decaySec, float size, float damping, float mix);
void paintCloudsAtlasFrame(juce::Graphics& g, juce::Rectangle<float> frame, int frameIndex, int frameCount, float mix,
                            int seed);

void paintChorusAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float rateHz, float depthMs, float mix,
                          float phase01);
void paintTapeAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float rate, float depth, float mix,
                        float phase01);
void paintReverbAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float decaySec, float size, float damping,
                          float mix, float phase01);
void paintCloudsAnimation(juce::Graphics& g, juce::Rectangle<float> dest, float mix, int seed, float phase01);

} // namespace pw8::plugin::ui::preview
