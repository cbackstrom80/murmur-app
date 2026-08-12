#pragma once

#include "EnvelopeCurveMath.h"
#include "LfoPreviewSampler.h"
#include "WireframeCanvas.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui::wireframe
{
    /// MOD tab wireframe: LFO1 waveform, amp-env snippet, and active route diagram.
    class ModRoutingWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        ModRoutingWireframeView(PatchworkEightProcessor& processor, juce::AudioProcessorValueTreeState& apvts);
        ~ModRoutingWireframeView() override;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        lfo::LfoWaveform lfoWaveform_ = lfo::LfoWaveform::Sine;
        float lfoRateHz_ = 2.0f;
        float lfoAnimPhase_ = 0.0f;
        EnvelopePreviewParams envParams_{};
    };

} // namespace pw8::plugin::ui::wireframe
