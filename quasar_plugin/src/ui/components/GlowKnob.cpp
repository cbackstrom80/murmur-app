#include "GlowKnob.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../theme/ObsidianRotary.h"

namespace pw8::quasar::ui
{
    namespace
    {
        constexpr int kNameLabelHeight = 14;
        constexpr int kTextBoxHeight = 16;
    } // namespace

    juce::String GlowKnob::FormattedSlider::getTextFromValue(const double value)
    {
        if (valueToText)
            return valueToText(static_cast<float>(value));
        return juce::String(value, 2);
    }

    GlowKnob::GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                       const juce::String& name, std::function<juce::String(float)> valueToText,
                       juce::Colour accentColour)
    {
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setRotaryParameters(rotary::kStartAngle, rotary::kEndAngle, true);
        slider_.valueToText = std::move(valueToText);
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 76, kTextBoxHeight);
        slider_.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
        slider_.setColour(juce::Slider::textBoxOutlineColourId, palette::kBorder);
        slider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
        if (!accentColour.isTransparent())
            slider_.setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        addAndMakeVisible(slider_);

        nameLabel_.setText(name.toUpperCase(), juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        nameLabel_.setFont(fonts::label(11.0f));
        addAndMakeVisible(nameLabel_);

        attachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider_);
        slider_.getProperties().set("maxDialDiameter", 72);
    }

    void GlowKnob::resized()
    {
        auto bounds = getLocalBounds();
        nameLabel_.setBounds(bounds.removeFromBottom(kNameLabelHeight));
        slider_.setBounds(bounds);
    }

} // namespace pw8::quasar::ui
