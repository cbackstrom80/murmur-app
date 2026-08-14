#pragma once

#include <juce_graphics/juce_graphics.h>

#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui::edgeicons
{
    /// Monoline path for one edge type, normalized to `bounds`.
    [[nodiscard]] juce::Path pathForEdge(algorithm::EdgeType type, juce::Rectangle<float> bounds);

    /// Draw icon centered in `bounds` with given stroke colour.
    void drawEdgeIcon(juce::Graphics& g, algorithm::EdgeType type, juce::Rectangle<float> bounds, juce::Colour colour,
                      float strokeWidth = 1.3f);

} // namespace pw8::plugin::ui::edgeicons
