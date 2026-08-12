#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

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

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refreshParams();

        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t envIndex_;
        EnvelopePreviewParams params_{};
    };

} // namespace pw8::plugin::ui::wireframe
