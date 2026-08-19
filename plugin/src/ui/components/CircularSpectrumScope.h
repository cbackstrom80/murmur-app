#pragma once

#include <array>

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/MurmurProcessor.h"
#include "ui/ScopeViewMode.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Radial FFT scope for compact mode — log-spaced bins around 360°, peak hold, glow stroke.
    class CircularSpectrumScope : public juce::Component, private juce::Timer
    {
    public:
        explicit CircularSpectrumScope(MurmurProcessor& processor);
        ~CircularSpectrumScope() override;

        void setViewMode(ScopeViewMode mode);
        [[nodiscard]] ScopeViewMode getViewMode() const noexcept { return viewMode_; }

        void paint(juce::Graphics& g) override;

    private:
        static constexpr int kFftOrder = 10;
        static constexpr int kFftSize = 1 << kFftOrder;
        static constexpr int kVuCaptureSize = 512;
        static constexpr int kScopeBins = 64;
        static constexpr float kMinFreqHz = 20.0f;
        static constexpr float kMaxFreqHz = 20000.0f;
        static constexpr float kMinDb = -62.0f;
        static constexpr float kMaxDb = 0.0f;

        void timerCallback() override;
        void rebuildWindow() noexcept;
        [[nodiscard]] bool pullAndAnalyseFft() noexcept;
        [[nodiscard]] bool pullAndAnalyseVu() noexcept;
        [[nodiscard]] float binMagnitudeDb(float binIndex) const noexcept;
        void paintSpectrumRing(juce::Graphics& g, juce::Point<float> centre, float innerR, float outerR) const;
        void paintPeakHold(juce::Graphics& g, juce::Point<float> centre, float innerR, float outerR) const;
        void paintVuArc(juce::Graphics& g, juce::Point<float> centre, float innerR, float outerR) const;
        [[nodiscard]] juce::Path buildOutlinePath(juce::Point<float> centre, float innerR, float outerR) const;

        MurmurProcessor& processor_;
        ScopeViewMode viewMode_ = ScopeViewMode::Fft;
        juce::dsp::FFT fft_{kFftOrder};
        std::array<float, static_cast<std::size_t>(kFftSize * 2)> fftData_{};
        std::array<float, static_cast<std::size_t>(kFftSize)> captureBuffer_{};
        std::array<float, static_cast<std::size_t>(kVuCaptureSize)> vuCaptureBuffer_{};
        std::array<float, static_cast<std::size_t>(kFftSize)> window_{};
        float windowSum_ = 1.0f;
        std::array<float, static_cast<std::size_t>(kScopeBins)> targetLevels_{};
        std::array<float, static_cast<std::size_t>(kScopeBins)> displayLevels_{};
        std::array<float, static_cast<std::size_t>(kScopeBins)> peakHold_{};
        std::array<float, static_cast<std::size_t>(kScopeBins)> smoothed_{};
        scope::VuBallistics vu_;
        double sampleRate_ = 48000.0;
        float runningRms_ = 0.0f;
        float rmsEnvelope_ = 0.0f;
        float pulseGlow_ = 0.0f;
        bool hasFreshData_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CircularSpectrumScope)
    };

} // namespace pw8::plugin::ui
