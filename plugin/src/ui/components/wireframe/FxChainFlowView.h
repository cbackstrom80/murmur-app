#pragma once

#include <array>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../FxEffectPlayParams.h"

// Signal-flow strip: Layer → I1→I2→I3 → Master → M1→M2→M3→M4 → Out.
// Click a slot box to select it; shows abbreviated effect type per slot.
namespace pw8::plugin::ui::wireframe
{
    class FxChainFlowView : public juce::Component, private juce::Timer
    {
    public:
        explicit FxChainFlowView(juce::AudioProcessorValueTreeState& apvts);

        void setSlotPrefixes(const std::array<juce::String, 7>& prefixes);
        void setSelectedSlot(std::size_t index);
        std::function<void(std::size_t slotIndex)> onSlotSelected;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void resized() override;

    private:
        void timerCallback() override;
        [[nodiscard]] std::size_t slotIndexAt(juce::Point<int> pos) const;
        void paintSlotBox(juce::Graphics& g, juce::Rectangle<float> box, std::size_t slotIndex, int effectType,
                          float mix) const;

        juce::AudioProcessorValueTreeState& apvts_;
        std::array<juce::String, 7> prefixes_{};
        std::array<int, 7> effectTypes_{};
        std::array<float, 7> mixValues_{};
        bool insertsPostFader_ = true;
        std::size_t selectedSlot_ = 0;
    };

} // namespace pw8::plugin::ui::wireframe
