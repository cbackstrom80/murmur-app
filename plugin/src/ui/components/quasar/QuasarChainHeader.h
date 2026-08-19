#pragma once

#include <array>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../GlowRingButton.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// M1–M4 chain breadcrumb + preset title + slot bypass (Figma `102:4` header row).
    class QuasarChainHeader : public juce::Component
    {
    public:
        QuasarChainHeader(PatchworkEightProcessor& processor, juce::AudioProcessorValueTreeState& apvts);

        std::function<void(std::size_t)> onSlotSelected;

        void bindSlot(std::size_t globalFxSlotIndex);
        void refresh();

        void resized() override;
        void paint(juce::Graphics& g) override;

    private:
        [[nodiscard]] juce::String slotParamPrefix(std::size_t globalFxSlotIndex) const;
        [[nodiscard]] std::size_t masterLocalIndex(std::size_t globalFxSlotIndex) const;
        void selectSlot(std::size_t globalFxSlotIndex);
        void toggleBypass();

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t slotIndex_ = 5;

        juce::Label presetLabel_;
        std::array<std::unique_ptr<GlowRingButton>, 4> slotPills_{};
        std::unique_ptr<GlowRingButton> quasarPill_;
        GlowRingButton bypassButton_{"BYPASS OFF"};
    };

} // namespace pw8::plugin::ui
