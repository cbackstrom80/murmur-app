#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    /// Filter magnitude-response wireframe — mode, cutoff, and resonance from APVTS.
    class FilterWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit FilterWireframeView(juce::AudioProcessorValueTreeState& apvts);
        ~FilterWireframeView() override;

        void setParamIds(const juce::String& enabledId, const juce::String& modeId, const juce::String& cutoffId,
                          const juce::String& resonanceId);

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::AudioProcessorValueTreeState& apvts_;
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
