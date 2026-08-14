#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::interstellar
{
    [[nodiscard]] bool isInterstellarCategory(const juce::String& category);

    /// Procedural HUD frame: coordinate tick marks + optional "INTERSTELLAR" capsule.
    void paintHudBadge(juce::Graphics& g, juce::Rectangle<float> bounds, bool showCapsule = true);

} // namespace pw8::plugin::ui::interstellar
