#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "../../visualizer/MurmurVisualizerComponent.h"
#include "../../visualizer/PreviewSurface.h"
#include "EnvelopeCurveMath.h"
#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    /// Classic DAHDSR envelope curve preview — time-normalized ADSR path with the
    /// same curve shaping as envelope::DahdsrEnvelope.
    class EnvelopeCurveView : public WireframeCanvas, private juce::Timer
    {
    public:
        EnvelopeCurveView(juce::AudioProcessorValueTreeState& apvts, std::size_t envIndex);
        ~EnvelopeCurveView() override;

        void attachVisualizerBus(murmur8::AudioVisualizerBus& bus);

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void refreshParams();
        void syncGlPreview();

        juce::AudioProcessorValueTreeState& apvts_;
        murmur8::AudioVisualizerBus* visualizerBus_ = nullptr;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> glPlot_;
        preview::PreviewSurface previewSurface_;
        juce::Rectangle<int> plotBounds_;
        std::size_t envIndex_;
        EnvelopePreviewParams params_{};
    };

} // namespace pw8::plugin::ui::wireframe
