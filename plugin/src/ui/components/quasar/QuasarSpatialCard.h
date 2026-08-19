#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../GlowKnob.h"
#include "../GlowRingButton.h"
#include "../SectionPanel.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// SPATIAL card — room presets, air distance, HP crossfeed toggle (Figma `102:4`).
    class QuasarSpatialCard : public juce::Component
    {
    public:
        QuasarSpatialCard(MurmurProcessor& processor, juce::AudioProcessorValueTreeState& apvts);

        void bindSlot(std::size_t globalFxSlotIndex);
        void refresh();

        void resized() override;

    private:
        [[nodiscard]] juce::String slotParamPrefix() const;
        [[nodiscard]] float readParam(const juce::String& suffix, float fallback) const;
        void setParam(const juce::String& suffix, float value);
        void applyRoomPreset(float roomAmount, float roomSize);
        void stylePill(juce::TextButton& btn, const juce::String& text);
        void highlightPill(juce::TextButton& btn, bool active);

        MurmurProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t slotIndex_ = 5;
        SectionPanel panel_{"SPATIAL", juce::Colour(0xffe040fb)};

        juce::Label roomHeader_;
        std::array<juce::TextButton, 3> roomPills_{};
        std::unique_ptr<GlowKnob> airKnob_;
        GlowRingButton hpCompButton_{"HP COMP OFF"};
    };

} // namespace pw8::plugin::ui
