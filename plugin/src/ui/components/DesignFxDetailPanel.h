#pragma once

#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "WireframePanel.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// DESIGN FX tab — spec-driven detail editing for Reverb, EQ, and Chorus (MVP).
    class DesignFxDetailPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit     DesignFxDetailPanel(PatchworkEightProcessor& processor);

        void resized() override;

    private:
        void selectSlot(std::size_t index);
        void rebuildKnobs();
        void timerCallback() override;
        [[nodiscard]] juce::String slotParamPrefix(std::size_t slotIndex) const;
        [[nodiscard]] juce::String slotShortLabel(std::size_t slotIndex) const;

        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t selectedSlot_ = 0;
        juce::Label slotHint_{"", "Slot:"};
        std::array<juce::TextButton, 7> slotButtons_{
            juce::TextButton{"I1"}, juce::TextButton{"I2"}, juce::TextButton{"I3"},
            juce::TextButton{"M1"}, juce::TextButton{"M2"}, juce::TextButton{"M3"},
            juce::TextButton{"M4"},
        };
        juce::Label typeLabel_;
        juce::Label deferredLabel_;
        WireframePanel detailFrame_{"FX PARAMS"};
        juce::Viewport knobViewport_;
        juce::Component knobContainer_;
        std::vector<std::unique_ptr<GlowKnob>> paramKnobs_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignFxDetailPanel)
    };

} // namespace pw8::plugin::ui
