#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "WireframeCanvas.h"
#include "../../visualizer/PreviewSurface.h"

namespace pw8::plugin::ui
{
    class DesignFxUiState;
}

namespace pw8::plugin
{
    class PatchworkEightProcessor;
}

namespace pw8::plugin::ui::wireframe
{
    /// Figma `murmur-fx-*` hero visualizer — right-hand panel beside the 280px knob grid.
    class DesignFxHeroViz : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit DesignFxHeroViz(juce::AudioProcessorValueTreeState& apvts);
        ~DesignFxHeroViz() override;

        void bindChip(std::size_t chipIndex, const juce::String& paramPrefix, PatchworkEightProcessor* processor,
                      int engineSlot);
        void setDesignFxUiState(pw8::plugin::ui::DesignFxUiState* uiState);

        std::function<void()> onUiPreferenceChanged;
        void setDesignModePill(const juce::String& pill);
        void paint(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        enum class EqDragBand
        {
            None,
            Low,
            Mid,
            High,
        };

        void timerCallback() override;
        void refreshCachedParams();
        [[nodiscard]] float eqDbFromPlotY(float y) const;
        [[nodiscard]] float eqPlotYFromDb(float db) const;
        [[nodiscard]] juce::Point<float> eqBandHandle(EqDragBand band) const;
        [[nodiscard]] EqDragBand hitEqBand(juce::Point<float> pt) const;
        void setEqGainParam(const char* suffix, float db);
        void setEqFloatParam(const char* suffix, float value, float minV, float maxV);
        [[nodiscard]] float eqFreqToT(float hz) const;
        [[nodiscard]] float eqTToFreq(float t) const;
        [[nodiscard]] float eqFreqForBand(EqDragBand band) const;
        [[nodiscard]] float constrainEqBandFreq(EqDragBand band, float freqHz) const;
        void updateLimiterAnimation();
        void rebuildEqAnalyzerWindow();
        void updateEqAnalyzerSpectrum();
        void paintEqAnalyzerSpectrum(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintEqHandles(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintEqParamStrip(juce::Graphics& g, juce::Rectangle<float> strip) const;

        struct EqAnalyzerState;
        std::unique_ptr<EqAnalyzerState> eqAnalyzer_;
        bool eqAnalyzerReady_ = false;

        juce::AudioProcessorValueTreeState& apvts_;
        pw8::plugin::ui::DesignFxUiState* uiState_ = nullptr;
        PatchworkEightProcessor* processor_ = nullptr;
        std::size_t chipIndex_ = 1;
        juce::String paramPrefix_;
        int engineSlot_ = -1;
        int effectType_ = 1;
        float mix_ = 1.0f;

        float driveDb_ = 6.0f;
        float chorusRateHz_ = 0.5f;
        float chorusDepthMs_ = 4.0f;
        float chorusBaseDelayMs_ = 12.0f;
        float tapeDriftRate_ = 0.3f;
        float tapeDriftDepth_ = 1.5f;
        float tapeDriveDb_ = 3.0f;
        float freqShiftHz_ = 7.0f;
        float freqShiftFeedback_ = 0.55f;
        float freqShiftLowCutHz_ = 120.0f;
        float fractalMorph_ = 0.0f;
        float reverbDecaySec_ = 2.0f;
        float reverbSize_ = 1.0f;
        float reverbDamping_ = 0.6f;
        float reverbPreDelayMs_ = 20.0f;
        float eqLowDb_ = 0.0f;
        float eqMidDb_ = 0.0f;
        float eqHighDb_ = 0.0f;
        float eqLowFreqHz_ = 200.0f;
        float eqMidFreqHz_ = 1000.0f;
        float eqHighFreqHz_ = 6000.0f;
        float eqMidQ_ = 0.8f;
        float compThresholdDb_ = -18.0f;
        float compRatio_ = 4.0f;
        float compKneeDb_ = 6.0f;
        float compGrDb_ = 0.0f;
        float compGrSmoothedDb_ = 0.0f;
        float compGrPeakHoldDb_ = 0.0f;
        float limiterCeilingDb_ = -0.2f;
        float limiterReleaseMs_ = 50.0f;
        float limiterAnimPhase_ = 0.0f;
        float limiterInputPeak_ = 0.0f;
        float limiterPeakHold_ = 0.0f;
        bool limiterClipping_ = false;
        int vocoderBands_ = 16;
        float vocoderFormant_ = 0.5f;
        float vocoderSibilance_ = 0.0f;
        float cloudsDensity_ = 0.35f;
        float cloudsGrainSizeMs_ = 80.0f;
        float cloudsPitch_ = 1.0f;
        float cloudsFreeze_ = 0.0f;
        int cloudsMode_ = 0;
        mutable float cloudsAnimPhase_ = 0.0f;
        float heroAnimPhase_ = 0.0f;
        preview::PreviewSurface heroSurface_;
        juce::String designModePill_;
        int reverbCharacter_ = 0;
        int saturationCharacter_ = 0;
        int compCharacter_ = 0;
        EqDragBand eqDragBand_ = EqDragBand::None;
        bool eqDragQMode_ = false;
        juce::Rectangle<float> eqPlotBounds_;
        mutable juce::Rectangle<float> eqAnalyzerToggleBounds_;
        mutable juce::Rectangle<float> limTruePeakToggleBounds_;
        juce::Rectangle<int> plotBounds_;

        [[nodiscard]] float uiKnob(std::size_t index, float fallback = 0.5f) const;
        [[nodiscard]] float readParam(const char* suffix, float fallback = 0.0f) const;
        [[nodiscard]] float readCompressorGrDb() const;

        static void paintPlotGrid(juce::Graphics& g, juce::Rectangle<float> plot);
        void paintSaturationTransfer(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintChorusSpatializer(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintTapeDrift(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintMoodResponse(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintFreqShiftBode(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintFractalCloud(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintReverbDecay(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintEqCurve(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintCompressorDynamics(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintLimiterCeiling(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintVocoderSpectrum(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintCloudsGranular(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintBypassHero(juce::Graphics& g, juce::Rectangle<float> plot) const;
    };

} // namespace pw8::plugin::ui::wireframe
