#pragma once

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../ScopeVuMeter.h"
#include "../visualizer/MurmurVisualizerComponent.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Time-domain oscilloscope fed by the master-bus audio tap (~30 Hz refresh).
    class OscilloscopeView : public juce::Component, private juce::Timer
    {
    public:
        explicit OscilloscopeView(PatchworkEightProcessor& processor);

        ~OscilloscopeView() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        /// Figma `murmur-desktop-play-mode` oscilloscope (36:35).
        void setDesktopPlayModeLayout(bool desktopPlayMode);

        /// Figma `ipad-play-view` scope card (4:2472) — mode pills + side-by-side upper deck.
        void setIpadPlayLayout(bool ipadPlayLayout);

        /// Figma `murmur-play-compact` scope-panel oscilloscope (4:1152).
        void setCompactLayout(bool compactLayout);

        [[nodiscard]] float getLastPeakLinear() const noexcept { return lastPeakLinear_; }

    private:
        void timerCallback() override;
        void paintDesktopPlayMode(juce::Graphics& g, juce::Rectangle<float> bounds);
        void paintIpadPlayMode(juce::Graphics& g, juce::Rectangle<float> bounds);
        void paintCompactMode(juce::Graphics& g, juce::Rectangle<float> bounds);
        void paintWaveform(juce::Graphics& g, juce::Rectangle<float> plot) const;
        void paintScopeModePills(juce::Graphics& g, juce::Rectangle<float> bounds) const;
        void positionGlPlot(juce::Rectangle<float> plot, murmur8::MurmurVisualizerComponent::Mode mode);
        [[nodiscard]] int scopeModePillAt(juce::Point<int> pos) const;

        enum class ScopeDisplayMode
        {
            Live = 0,
            Spectrum,
            Vector,
        };

        PatchworkEightProcessor& processor_;
        bool desktopPlayMode_ = false;
        bool ipadPlayLayout_ = false;
        bool compactLayout_ = false;
        ScopeDisplayMode scopeDisplayMode_ = ScopeDisplayMode::Live;
        std::array<juce::Rectangle<int>, 3> scopeModePillBounds_{};
        scope::VuBallistics leftVu_;
        scope::VuBallistics rightVu_;
        float lastPeakLinear_ = 0.0f;
        static constexpr int kCaptureSize = 512;
        std::array<float, static_cast<std::size_t>(kCaptureSize)> capture_{};
        int captureCount_ = 0;
        bool hasData_ = false;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> glPlot_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeView)
    };

} // namespace pw8::plugin::ui
