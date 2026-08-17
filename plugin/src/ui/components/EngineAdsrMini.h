#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "wireframe/EnvelopeCurveMath.h"

namespace pw8::plugin::ui
{
    /// Compact per-engine ADSR — Figma envelope-group (4:107): preview box + A/D/S/R ticks.
    class EngineAdsrMini : public juce::Component, private juce::Timer
    {
    public:
        EngineAdsrMini(PatchworkEightProcessor& processor, int engineIndex);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;

    private:
        enum class TickKind
        {
            None,
            Attack,
            Decay,
            Sustain,
            Release,
        };

        void timerCallback() override;
        void refreshFromApvts();
        [[nodiscard]] TickKind tickAt(juce::Point<int> pos) const;
        void applyTickDrag(TickKind kind, juce::Point<int> pos);
        void paintPreview(juce::Graphics& g, juce::Rectangle<float> bounds);
        void paintTicks(juce::Graphics& g, juce::Rectangle<int> bounds);

        PatchworkEightProcessor& processor_;
        const int engineIndex_;
        wireframe::EnvelopePreviewParams params_{};
        TickKind activeTick_ = TickKind::None;

        std::array<juce::Rectangle<int>, 4> tickLayout_{};
    };

} // namespace pw8::plugin::ui
