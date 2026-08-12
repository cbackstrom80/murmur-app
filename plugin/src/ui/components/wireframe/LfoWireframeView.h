#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "LfoPreviewSampler.h"
#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    class LfoWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit LfoWireframeView(juce::AudioProcessorValueTreeState& apvts, std::size_t lfoIndex = 0);
        ~LfoWireframeView() override;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t lfoIndex_;
        lfo::LfoWaveform waveform_ = lfo::LfoWaveform::Sine;
        lfo::LfoMode mode_ = lfo::LfoMode::Free;
        float rateHz_ = 2.0f;
        float phaseOffset_ = 0.0f;
        float animPhase_ = 0.0f;
    };

} // namespace pw8::plugin::ui::wireframe
