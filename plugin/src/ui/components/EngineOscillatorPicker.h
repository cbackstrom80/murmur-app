#pragma once

#include <array>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/oscillator/ResonatorOscillator.hpp"
#include "EngineOscContextThumb.h"
#include "wireframe/OscPreviewSampler.h"

namespace pw8::plugin::ui
{
    /// Figma `oscillator-picker`: 14px type strip + 22px sub-picker + 80px context visualizer (design v2).
    class EngineOscillatorPicker : public juce::Component, private juce::Timer
    {
    public:
        EngineOscillatorPicker(PatchworkEightProcessor& processor, int engineIndex);

        void setPlayBoardCompactMode(bool compact);
        void setDesignModeV2Layout(bool designMode);
        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;

        std::function<void()> onWavetableLabRequested;

    private:
        void timerCallback() override;
        void refreshPreviews();
        void advanceAnimation();
        [[nodiscard]] bool isEngineLive() const;
        [[nodiscard]] algorithm::EngineType currentEngineType() const;
        [[nodiscard]] int numTypePills() const;
        [[nodiscard]] algorithm::EngineType engineForPill(int pillIndex) const;
        [[nodiscard]] int pillIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] int cellIndexAt(juce::Point<int> pos) const;
        void setEngineType(algorithm::EngineType type);
        void activateContextCell(int cellIndex);
        [[nodiscard]] float previewSample(const std::array<float, wireframe::kPreviewPoints>& buf, float t,
                                          float phaseOffset, float amp) const;
        void paintTypeStrip(juce::Graphics& g);
        void paintContextGrid(juce::Graphics& g);
        void paintSubPicker(juce::Graphics& g);
        void paintSubPickerPills(juce::Graphics& g, juce::Rectangle<int> area,
                                 const std::vector<const char*>& labels, int activeIndex);
        void paintSubPickerLabeled(juce::Graphics& g, juce::Rectangle<int> area, const char* label,
                                   const std::vector<const char*>& labels, int activeIndex);
        void paintSubPickerCycler(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title,
                                  const juce::String& indexText);
        void paintSubPickerSingleButton(juce::Graphics& g, juce::Rectangle<int> area, const char* label,
                                        const char* buttonText);
        [[nodiscard]] int subPickerPillIndexAt(juce::Point<int> pos) const;
        void activateSubPickerPill(int pillIndex);
        void stepSubPickerCycler(int delta);
        void paintWaveCell(juce::Graphics& g, juce::Rectangle<float> cell, int cellIndex, bool selected,
                           const std::array<float, wireframe::kPreviewPoints>& samples, const char* label);
        void paintWavetableWaveCell(juce::Graphics& g, juce::Rectangle<float> cell, int cellIndex, bool selected,
                                    float framePos01, const char* label, bool granularOverlay);
        void paintBarCell(juce::Graphics& g, juce::Rectangle<float> cell, bool selected,
                         const std::function<float(int)>& heightAt, int barCount);
        void paintPlayBoardStub(juce::Graphics& g);
        void syncContextThumb();

        PatchworkEightProcessor& processor_;
        bool playBoardCompactMode_ = false;
        bool designModeV2Layout_ = false;
        const int engineIndex_;
        int activeContextCell_ = 0;
        int activeSubPickerIndex_ = 0;
        float animPhase_ = 0.0f;
        float motionGain_ = 1.0f;
        bool engineLive_ = false;
        int previewRefreshCounter_ = 0;

        std::array<std::array<float, wireframe::kPreviewPoints>, 4> classicPreviews_{};
        std::array<std::array<float, wireframe::kPreviewPoints>, 4> fmCarrierPreviews_{};
        std::array<std::array<float, wireframe::kPreviewPoints>, 4> fmModPreviews_{};
        std::array<float, wireframe::kPreviewPoints> fmLiveCarrier_{};
        std::array<float, wireframe::kPreviewPoints> fmLiveMod_{};
        std::array<std::array<float, wireframe::kPreviewPoints>, 4> phaseOutPreviews_{};
        std::array<float, wireframe::kMaxPreviewPartials> additiveHeights_{};
        int additiveBarCount_ = 8;
        std::array<float, oscillator::ResonatorOscillator::kMaxModes> resonatorHeights_{};
        int resonatorBarCount_ = 6;
        std::array<std::array<float, wireframe::kPreviewPoints>, 4> noisePreviews_{};
        std::vector<juce::Rectangle<int>> typePillLayout_;
        std::vector<juce::Rectangle<int>> subPillLayout_;
        juce::Rectangle<int> subPickerBounds_;
        juce::Rectangle<int> contextPreviewBounds_;
        juce::Rectangle<int> subPickerLeftArrow_;
        juce::Rectangle<int> subPickerRightArrow_;
        bool subPickerUsesCycler_ = false;
        std::array<juce::Rectangle<int>, 4> cellLayout_{};
        EngineOscContextThumb contextThumb_;
    };

} // namespace pw8::plugin::ui
