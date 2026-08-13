#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::branding
{
    /// Soft violet from the Murmur brand kit — used for glow accents.
    [[nodiscard]] juce::Colour glowColour() noexcept;

    /// Organic M / eight-node mark (black keyed out).
    [[nodiscard]] juce::Image getMarkIcon();

    [[nodiscard]] int wordmarkWidth() noexcept;
    [[nodiscard]] int headerBarHeight() noexcept;

    void paintMarkGlow(juce::Graphics& g, const juce::Image& mark, juce::Rectangle<float> bounds) noexcept;

} // namespace pw8::plugin::ui::branding
