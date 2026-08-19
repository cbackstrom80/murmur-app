#pragma once

#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../GlowKnob.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Seven primary QUASAR knobs with mod-matrix drop targets (Figma `102:4` hero row).
    class QuasarPrimaryKnobRow : public juce::Component
    {
    public:
        QuasarPrimaryKnobRow(PatchworkEightProcessor& processor, juce::AudioProcessorValueTreeState& apvts);

        void rebuild(std::size_t globalFxSlotIndex);

        void resized() override;

    private:
        [[nodiscard]] std::size_t masterLocalIndex(std::size_t globalFxSlotIndex) const;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        std::vector<std::unique_ptr<GlowKnob>> knobs_;
    };

} // namespace pw8::plugin::ui
