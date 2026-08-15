#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::quasar::ui
{
    class GlowKnob : public juce::Component
    {
    public:
        GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& name,
                 std::function<juce::String(float)> valueToText = nullptr,
                 juce::Colour accentColour = juce::Colours::transparentBlack);

        void resized() override;

    private:
        class FormattedSlider : public juce::Slider
        {
        public:
            std::function<juce::String(float)> valueToText;
            juce::String getTextFromValue(double value) override;
        };

        FormattedSlider slider_;
        juce::Label nameLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;
    };

} // namespace pw8::quasar::ui
