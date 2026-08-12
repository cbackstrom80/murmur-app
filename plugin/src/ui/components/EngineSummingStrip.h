#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

// GLOBAL-scope mixer: visualize and set per-operator Level before the layer bus.
namespace pw8::plugin::ui
{
    class EngineSummingStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit EngineSummingStrip(PatchworkEightProcessor& processor);
        ~EngineSummingStrip() override;

        void setHighlightedEngine(int engineIndex);
        std::function<void(int engineIndex)> onEngineClicked;

        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        [[nodiscard]] juce::Rectangle<float> faderArea() const;
        [[nodiscard]] juce::Rectangle<float> faderColumnBounds(int engineIndex) const;
        [[nodiscard]] int faderIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] float readLevel(int engineIndex) const;
        void setLevelFromY(int engineIndex, float yInColumn);
        void adjustLevelAtMouse(juce::Point<int> pos);

        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Engine Sum"};
        juce::Label helpLabel_;
        std::unique_ptr<GlowKnob> layerGainKnob_;
        std::unique_ptr<GlowKnob> masterGainKnob_;
        int highlightedEngine_ = 0;
        int dragEngine_ = -1;
        std::array<float, 8> cachedLevels_{};
        std::array<int, 8> cachedEngines_{};
    };

} // namespace pw8::plugin::ui
