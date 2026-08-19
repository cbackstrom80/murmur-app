#pragma once

#include <array>
#include <functional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../GlowKnob.h"
#include "../SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// BINAURAL ENGINE card — HRTF/mode pills, mono HPF, phase correlation (Figma `102:4`).
    class QuasarEngineCard : public juce::Component
    {
    public:
        QuasarEngineCard(PatchworkEightProcessor& processor, juce::AudioProcessorValueTreeState& apvts);

        void bindSlot(std::size_t globalFxSlotIndex);
        void tick();
        void refresh();

        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;

    private:
        [[nodiscard]] juce::String slotParamPrefix() const;
        [[nodiscard]] float readParam(const juce::String& suffix, float fallback) const;
        void setParam(const juce::String& suffix, float value);
        void setIntParam(const juce::String& suffix, int value);
        void stylePill(juce::TextButton& btn, const juce::String& text);
        void highlightPill(juce::TextButton& btn, bool active, juce::Colour accent);
        void refreshPills();
        void updateCorrelationFromScope();

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t slotIndex_ = 5;
        SectionPanel panel_{"BINAURAL ENGINE", juce::Colour(0xff00c8ff)};

        juce::Label hrtfHeader_;
        std::array<juce::TextButton, 3> hrtfPills_{};
        juce::Label modeHeader_;
        std::array<juce::TextButton, 3> modePills_{};
        std::unique_ptr<GlowKnob> monoBelowKnob_;
        juce::Label correlationLabel_;
        juce::Rectangle<int> correlationBounds_;
        float correlationDisplay_ = 1.0f;
        std::array<float, 512> scopeLeft_{};
        std::array<float, 512> scopeRight_{};
    };

} // namespace pw8::plugin::ui
