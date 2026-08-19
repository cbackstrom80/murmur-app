#pragma once

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/MurmurProcessor.h"
#include "wireframe/OscPreviewSampler.h"

namespace pw8::plugin::ui
{
    /// 2×2 classic waveform picker with live cycle previews (Figma engine-card osc grid).
    class EngineWaveformSelector : public juce::Component, private juce::Timer
    {
    public:
        EngineWaveformSelector(MurmurProcessor& processor, int engineIndex);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        void timerCallback() override;
        void refreshPreviews();
        void advanceAnimation();
        [[nodiscard]] bool isEngineLive() const;
        [[nodiscard]] float previewSample(const std::array<float, wireframe::kPreviewPoints>& buf, float t,
                                          float phaseOffset, float amp) const;
        void setWaveformOrdinal(int ordinal);
        [[nodiscard]] int cellIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] juce::Rectangle<float> cellBounds(int index) const;

        MurmurProcessor& processor_;
        const int engineIndex_;
        int activeWaveform_ = 2;
        float animPhase_ = 0.0f;
        float motionGain_ = 1.0f;
        bool engineLive_ = false;
        int previewRefreshCounter_ = 0;
        std::array<std::array<float, wireframe::kPreviewPoints>, 4> previews_{};
        std::array<juce::Rectangle<int>, 4> cellLayout_{};
    };

} // namespace pw8::plugin::ui
