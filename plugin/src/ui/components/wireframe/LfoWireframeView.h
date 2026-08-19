#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "../../visualizer/MurmurVisualizerComponent.h"
#include "../../visualizer/PreviewSurface.h"
#include "LfoPreviewSampler.h"
#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    class LfoWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit LfoWireframeView(juce::AudioProcessorValueTreeState& apvts, std::size_t lfoIndex = 0);
        ~LfoWireframeView() override;

        void attachVisualizerBus(murmur8::AudioVisualizerBus& bus);
        void setLfoIndex(std::size_t lfoIndex);

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void syncGlPreview();

        juce::AudioProcessorValueTreeState& apvts_;
        murmur8::AudioVisualizerBus* visualizerBus_ = nullptr;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> glPlot_;
        preview::PreviewSurface previewSurface_;
        juce::Rectangle<int> plotBounds_;
        std::size_t lfoIndex_;
        lfo::LfoWaveform waveform_ = lfo::LfoWaveform::Sine;
        lfo::LfoMode mode_ = lfo::LfoMode::Free;
        float rateHz_ = 2.0f;
        float phaseOffset_ = 0.0f;
    };

} // namespace pw8::plugin::ui::wireframe
