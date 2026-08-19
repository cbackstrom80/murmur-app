#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "../../visualizer/MurmurVisualizerComponent.h"
#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    /// Signal-flow wireframe for one FX slot — GL shader preview driven by slot params.
    class FxWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit FxWireframeView(juce::AudioProcessorValueTreeState& apvts);
        ~FxWireframeView() override;

        void attachVisualizerBus(murmur8::AudioVisualizerBus& bus);
        void bindToSlot(const juce::String& paramPrefix);
        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void syncGlPreview();
        [[nodiscard]] int fxKindForEffectType() const;

        juce::AudioProcessorValueTreeState& apvts_;
        murmur8::AudioVisualizerBus* visualizerBus_ = nullptr;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> glPlot_;
        juce::String paramPrefix_;
        juce::Rectangle<int> plotBounds_;
        int effectType_ = 0;
        float mix_ = 1.0f;
    };

} // namespace pw8::plugin::ui::wireframe
