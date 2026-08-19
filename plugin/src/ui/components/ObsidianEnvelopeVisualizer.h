#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../visualizer/MurmurVisualizerComponent.h"
#include "wireframe/EnvelopePathBuilder.h"

namespace pw8::plugin::ui
{
    class ObsidianEnvelopeVisualizer : public juce::Component, private juce::Timer
    {
    public:
        ObsidianEnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts, std::size_t envIndex = 0);
        ~ObsidianEnvelopeVisualizer() override;

        void attachVisualizerBus(murmur8::AudioVisualizerBus& bus);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;

    private:
        enum class HandleKind
        {
            None,
            DelayEnd,
            AttackPeak,
            HoldEnd,
            DecaySustain,
            ReleaseEnd,
        };

        void timerCallback() override;
        void refreshFromApvts();
        [[nodiscard]] juce::Rectangle<float> graphBounds() const;
        void rebuildLayout();
        [[nodiscard]] HandleKind handleAtPoint(juce::Point<float> pt) const;
        [[nodiscard]] juce::Point<float> handlePosition(HandleKind kind) const;
        void applyDrag(HandleKind kind, juce::Point<float> pt);
        void writeParam(const char* suffix, float realValue);
        void endActiveGestures();
        [[nodiscard]] juce::String paramId(const char* suffix) const;
        [[nodiscard]] float readParam(const char* suffix, float fallback) const;
        void updateHoverCursor(HandleKind kind);
        void paintGrid(juce::Graphics& g, juce::Rectangle<float> bounds) const;
        void paintHandles(juce::Graphics& g) const;
        void paintStageLabels(juce::Graphics& g) const;
        void syncGlPreview();

        juce::AudioProcessorValueTreeState& apvts_;
        std::unique_ptr<murmur8::MurmurVisualizerComponent> glPlot_;
        std::size_t envIndex_;
        wireframe::EnvelopePreviewParams params_{};
        wireframe::EnvelopeLayout layout_{};
        HandleKind activeHandle_ = HandleKind::None;
        HandleKind hoveredHandle_ = HandleKind::None;
        bool dragging_ = false;
        bool gestureActive_[6] = {};

        static constexpr float kHandleRadius = 7.0f;
        static constexpr float kHitRadius = 10.0f;
        static constexpr int kMargin = 6;
    };

} // namespace pw8::plugin::ui
