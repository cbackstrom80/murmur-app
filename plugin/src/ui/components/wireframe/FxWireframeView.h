#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    /// Signal-flow wireframe for one FX slot — schematic updates with `Type` and `Mix`.
    class FxWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit FxWireframeView(juce::AudioProcessorValueTreeState& apvts);
        ~FxWireframeView() override;

        void bindToSlot(const juce::String& paramPrefix);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::AudioProcessorValueTreeState& apvts_;
        juce::String paramPrefix_;
        int effectType_ = 0;
        float mix_ = 1.0f;

        static void paintEffectSchematic(juce::Graphics& g, juce::Rectangle<float> bounds, int type, float mix);
    };

} // namespace pw8::plugin::ui::wireframe
