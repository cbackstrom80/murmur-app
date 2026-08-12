#pragma once

#include "WireframeCanvas.h"
#include "pw8/oscillator/ResonatorOscillator.hpp"
#include "OscPreviewSampler.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui::wireframe
{
    class ClassicWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit ClassicWireframeView(PatchworkEightProcessor& processor);
        ~ClassicWireframeView() override;

        void showNode(int nodeIndex);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refresh();

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        std::array<std::array<float, kPreviewPoints>, 4> shapeRows_{};
        int activeShape_ = 0;
    };

    class FmWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit FmWireframeView(PatchworkEightProcessor& processor);
        ~FmWireframeView() override;

        void showNode(int nodeIndex);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refresh();

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        std::array<float, kPreviewPoints> carrier_{};
        std::array<float, kPreviewPoints> mod_{};
    };

    class AdditiveWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit AdditiveWireframeView(PatchworkEightProcessor& processor);
        ~AdditiveWireframeView() override;

        void showNode(int nodeIndex);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refresh();

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        std::array<float, kMaxPreviewPartials> heights_{};
        int partialCount_ = 0;
    };

    class PhaseShapeWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit PhaseShapeWireframeView(PatchworkEightProcessor& processor);
        ~PhaseShapeWireframeView() override;

        void showNode(int nodeIndex);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refresh();

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        std::array<float, kPreviewPoints> output_{};
        std::array<float, kPreviewPoints> warp_{};
    };

    class ResonatorWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit ResonatorWireframeView(PatchworkEightProcessor& processor);
        ~ResonatorWireframeView() override;

        void showNode(int nodeIndex);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refresh();

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        std::array<float, oscillator::ResonatorOscillator::kMaxModes> heights_{};
        int modeCount_ = 0;
    };

    class NoiseWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit NoiseWireframeView(PatchworkEightProcessor& processor);
        ~NoiseWireframeView() override;

        void showNode(int nodeIndex);
        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;
        void refresh();

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        std::array<float, kNoisePoints> samples_{};
    };

} // namespace pw8::plugin::ui::wireframe
