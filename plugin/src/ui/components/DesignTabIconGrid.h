#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::designtabicons
{
    enum class DesignTab
    {
        Graph = 0,
        Matrix,
        Fx,
        Wavetable,
    };

    [[nodiscard]] juce::Path pathForTab(DesignTab tab, juce::Rectangle<float> bounds);

    void drawTabIcon(juce::Graphics& g, DesignTab tab, juce::Rectangle<float> bounds, juce::Colour colour,
                     float strokeWidth = 1.4f);

} // namespace pw8::plugin::ui::designtabicons
