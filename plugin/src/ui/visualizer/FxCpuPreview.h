#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::preview
{

/// CPU wireframe / slot FX preview — mirrors DesignFxHeroViz without GL.
void paintFxWireframePreview(juce::Graphics& g, juce::Rectangle<float> plot,
                             juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix,
                             int effectType, float mix);

void paintQuasarFieldPreview(juce::Graphics& g, juce::Rectangle<float> plot,
                             juce::AudioProcessorValueTreeState& apvts, const juce::String& paramPrefix, float mix);

} // namespace pw8::plugin::ui::preview
