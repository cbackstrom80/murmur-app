#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "WireframeCanvas.h"

namespace pw8::plugin::ui::wireframe
{
    /// Blades dual-filter routing morph diagram (serial → parallel → crossfade).
    /// Figma `murmur-mi-ui-component-blades-routing-diagram` (`89:246`).
    class FilterRoutingWireframeView : public WireframeCanvas, private juce::Timer
    {
    public:
        explicit FilterRoutingWireframeView(juce::AudioProcessorValueTreeState& apvts);
        ~FilterRoutingWireframeView() override;

        /// When true, paints three stacked state rows (design Filter Lab). When false, compact morph strip (PLAY).
        void setStackedStatesLayout(bool stacked) noexcept;

        void paint(juce::Graphics& g) override;

    private:
        enum class RoutingStateRow
        {
            Serial,
            Parallel,
            Crossfade,
        };

        void timerCallback() override;
        void paintCompactMorph(juce::Graphics& g, juce::Rectangle<float> bounds);
        void paintStackedStates(juce::Graphics& g, juce::Rectangle<float> bounds);
        void paintStateRow(juce::Graphics& g, juce::Rectangle<float> rowBounds, RoutingStateRow state,
                           float emphasis) const;
        [[nodiscard]] float rowEmphasis(RoutingStateRow state) const noexcept;

        juce::AudioProcessorValueTreeState& apvts_;
        float routing_ = 0.0f;
        bool f1Enabled_ = false;
        bool f2Enabled_ = false;
        bool stackedStatesLayout_ = false;
    };

} // namespace pw8::plugin::ui::wireframe
