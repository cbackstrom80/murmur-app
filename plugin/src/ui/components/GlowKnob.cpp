#include "GlowKnob.h"

#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    juce::String GlowKnob::FormattedSlider::getTextFromValue(double value)
    {
        if (valueToText)
            return valueToText(static_cast<float>(value));
        // Deliberately not delegating to juce::Slider::getTextFromValue() here --
        // its default decimal-place count is derived from the parameter's step
        // interval in a way that's easy to fight with (setNumDecimalPlacesToDisplay
        // needs to survive SliderAttachment's own range setup), and this is simple
        // enough to just own directly: fixed 2-decimal display for every continuous
        // (non-formatter) knob.
        return juce::String(value, 2);
    }

    GlowKnob::GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                        const juce::String& name, std::function<juce::String(float)> valueToText)
    {
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f,
                                     true);
        slider_.valueToText = std::move(valueToText);
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, 16);
        slider_.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        slider_.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        slider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        addAndMakeVisible(slider_);

        nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        nameLabel_.setFont(juce::Font(juce::FontOptions(10.5f)));
        addAndMakeVisible(nameLabel_);

        attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId,
                                                                                               slider_);
    }

    void GlowKnob::resized()
    {
        auto bounds = getLocalBounds();
        nameLabel_.setBounds(bounds.removeFromBottom(14));
        slider_.setBounds(bounds);
    }

} // namespace pw8::plugin::ui
