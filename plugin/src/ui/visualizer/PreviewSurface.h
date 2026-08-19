#pragma once

#include <cstdint>
#include <functional>

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::preview
{

static constexpr float kPreviewSupersampleScale = 2.0f;

/// Layered preview: background (grid) + 2× baked data + live overlay.
class PreviewSurface
{
public:
    static constexpr float kSupersampleScale = kPreviewSupersampleScale;

    void setPlotBounds(juce::Rectangle<float> plotBounds) noexcept;
    void setDataKey(uint64_t key) noexcept;

    void invalidateBackground() noexcept { backgroundDirty_ = true; }
    void invalidateData() noexcept { dataDirty_ = true; }
    void invalidateAll() noexcept
    {
        backgroundDirty_ = true;
        dataDirty_ = true;
    }

    void paintBackground(juce::Graphics& g, const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw);

    void paintData(juce::Graphics& g, const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw);

    void paintOverlay(juce::Graphics& g, const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw) const;

    [[nodiscard]] juce::Rectangle<float> plotBounds() const noexcept { return plotBounds_; }

private:
    void ensureThemeCurrent() noexcept;
    void ensureBackgroundImage();
    void ensureDataImage(const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw);

    juce::Rectangle<float> plotBounds_;
    uint64_t dataKey_ = 0;
    uint64_t themeKey_ = 0;
    juce::Image backgroundLayer_;
    juce::Image dataLayer_;
    bool backgroundDirty_ = true;
    bool dataDirty_ = true;
};

[[nodiscard]] juce::Image renderPlotHiRes(juce::Rectangle<float> plotBounds,
                                           const std::function<void(juce::Graphics&, juce::Rectangle<float>)>& draw,
                                           float scale = kPreviewSupersampleScale);

} // namespace pw8::plugin::ui::preview
