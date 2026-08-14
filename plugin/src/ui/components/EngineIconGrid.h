#pragma once

#include <juce_graphics/juce_graphics.h>

#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui::engineicons
{
    /// Monoline path for one engine type, normalized to `bounds`.
    [[nodiscard]] juce::Path pathForEngine(algorithm::EngineType engine, juce::Rectangle<float> bounds);

    /// Draw icon centered in `bounds` with given stroke colour.
    void drawEngineIcon(juce::Graphics& g, algorithm::EngineType engine, juce::Rectangle<float> bounds,
                        juce::Colour colour, float strokeWidth = 1.4f);

} // namespace pw8::plugin::ui::engineicons
