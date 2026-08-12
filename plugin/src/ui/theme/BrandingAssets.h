#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::branding
{
    /// Electric blue pulled from the Starfighter logo art — used for glow accents.
    [[nodiscard]] juce::Colour glowColour() noexcept;

    /// Ship + targeting ring (top crop of mark logo, black keyed out).
    [[nodiscard]] juce::Image getShipIcon();

    [[nodiscard]] int wordmarkWidth() noexcept;
    [[nodiscard]] int headerBarHeight() noexcept;

    void paintShipGlow(juce::Graphics& g, const juce::Image& ship, juce::Rectangle<float> bounds) noexcept;

} // namespace pw8::plugin::ui::branding
