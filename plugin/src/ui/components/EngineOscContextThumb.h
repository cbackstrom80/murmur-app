#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/oscillator/ResonatorOscillator.hpp"
#include "wireframe/OscPreviewSampler.h"

namespace pw8::plugin::ui
{
    /// Snapshot of preview buffers + animation state for the 80px design-v2 context strip.
    struct EngineOscContextPreviewData
    {
        algorithm::EngineType engineType = algorithm::EngineType::Classic;
        bool engineLive = false;
        float animPhase = 0.0f;
        float motionGain = 1.0f;
        int activeSubPickerIndex = 0;

        const std::array<std::array<float, wireframe::kPreviewPoints>, 4>* classicPreviews = nullptr;
        const std::array<float, wireframe::kPreviewPoints>* fmLiveCarrier = nullptr;
        const std::array<float, wireframe::kPreviewPoints>* fmLiveMod = nullptr;
        const std::array<std::array<float, wireframe::kPreviewPoints>, 4>* phaseOutPreviews = nullptr;
        const std::array<float, wireframe::kMaxPreviewPartials>* additiveHeights = nullptr;
        int additiveBarCount = 8;
        const std::array<float, oscillator::ResonatorOscillator::kMaxModes>* resonatorHeights = nullptr;
        int resonatorBarCount = 6;
    };

    /// Figma design-v2 engine card context strip — 80px framed preview (`37:787`).
    class EngineOscContextThumb : public juce::Component
    {
    public:
        EngineOscContextThumb(PatchworkEightProcessor& processor, int engineIndex);

        void setPreviewData(const EngineOscContextPreviewData& data);
        void paint(juce::Graphics& g) override;

    private:
        static void paintFrame(juce::Graphics& g, juce::Rectangle<int> bounds);
        [[nodiscard]] float previewSample(const std::array<float, wireframe::kPreviewPoints>& buf, float t,
                                          float phaseOffset, float amp) const;

        void paintContent(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintClassic(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintWavetable(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintFm(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintAdditive(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintPhaseShape(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintGranular(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintNoise(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintResonator(juce::Graphics& g, juce::Rectangle<int> bounds);
        void paintExternal(juce::Graphics& g, juce::Rectangle<int> bounds);

        PatchworkEightProcessor& processor_;
        const int engineIndex_;
        EngineOscContextPreviewData preview_;
    };

} // namespace pw8::plugin::ui
