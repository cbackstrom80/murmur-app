#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::branding
{
    /// Soft violet from the Murmur brand kit — used for glow accents.
    [[nodiscard]] juce::Colour glowColour() noexcept;

    /// Organic M / eight-node mark (black keyed out).
    [[nodiscard]] juce::Image getMarkIcon();

    /// Figma `murmur-whale-logo` lockup (whale + MURMUR wordmark).
    [[nodiscard]] juce::Image getLogoLockup();

    /// The whale mark, transparent background -- this is MURMUR's
    /// default/primary brand asset (per user direction), used wherever the
    /// mark needs to stand alone at a larger size than the compact chrome
    /// lockup, e.g. the splash and key-activation screens. Note: verified
    /// visually (build screenshot) that the source PNG already bakes in a
    /// small "MURMUR" wordmark under the whale -- callers should NOT also
    /// draw a separate wordmark next to/under this image, or the brand
    /// name renders twice.
    [[nodiscard]] juce::Image getWhaleIcon();

    [[nodiscard]] int logoLockupWidth() noexcept;
    [[nodiscard]] int logoLockupHeight() noexcept;
    [[nodiscard]] int compactLogoWidth() noexcept;
    [[nodiscard]] int compactLogoHeight() noexcept;

    [[nodiscard]] int wordmarkWidth() noexcept;
    [[nodiscard]] int headerBarHeight() noexcept;

    void paintMarkGlow(juce::Graphics& g, const juce::Image& mark, juce::Rectangle<float> bounds) noexcept;

    void paintLogoLockup(juce::Graphics& g, juce::Rectangle<float> bounds) noexcept;

} // namespace pw8::plugin::ui::branding
