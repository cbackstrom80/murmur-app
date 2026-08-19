#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "../../visualizer/AudioVisualizerBus.h"
#include "../../visualizer/MurmurVisualizerComponent.h"
#include "../../visualizer/PreviewSurface.h"
#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    /// Filter magnitude-response wireframe — mode, cutoff, and resonance from APVTS.
    class FilterWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit FilterWireframeView(juce::AudioProcessorValueTreeState& apvts);
        ~FilterWireframeView() override;

        void attachVisualizerBus(murmur8::AudioVisualizerBus& bus);
        void setParamIds(const juce::String& enabledId, const juce::String& modeId, const juce::String& cutoffId,
                          const juce::String& resonanceId);

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
        juce::String enabledId_;
        juce::String modeId_;
        juce::String cutoffId_;
        juce::String resonanceId_;
        bool enabled_ = false;
        int mode_ = 0;
        float cutoffHz_ = 8000.0f;
        float resonance_ = 0.2f;
    };

} // namespace pw8::plugin::ui::wireframe
