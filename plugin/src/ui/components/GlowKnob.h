#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// The one control primitive PLAY mode is built from: a rotary knob (painted by
// ObsidianLookAndFeel), a name label underneath, and a value readout -- attached
// directly to an APVTS parameter, so automating it from a host visibly moves the
// same knob the user sees (the property docs/PLUGIN_ARCHITECTURE.md's whole
// "Automation" section exists to guarantee at the engine level; this is that
// guarantee's visible side).
namespace pw8::plugin::ui
{
    class GlowKnob : public juce::Component
    {
    public:
        /// `valueToText`, if given, formats the raw parameter value for the knob's
        /// text readout (e.g. mapping a discrete filter-mode float to "LOWPASS")
        /// instead of the raw number -- used for every enum-valued parameter this
        /// skin exposes, so PLAY mode never shows a bare "2.0" for a mode control.
        /// `accentColour`, if not transparent (the default), overrides
        /// ObsidianLookAndFeel's default cool-cyan value arc/pointer with a
        /// different color for this one knob -- the duotone mechanism (macros use
        /// the warm variant; see ObsidianPalette.h).
        GlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& name,
                  std::function<juce::String(float)> valueToText = nullptr,
                  juce::Colour accentColour = juce::Colours::transparentBlack);

        void resized() override;

    private:
        /// A juce::Slider subclass rather than relying on `textFromValueFunction`
        /// directly: overriding the virtual `getTextFromValue()` is a guaranteed,
        /// directly-verifiable formatting hook, and it doubles as the one place a
        /// sane default decimal precision gets applied for every continuous
        /// (non-formatter) knob -- fixing the "0.3635937"-style raw-float readout
        /// JUCE's own default would otherwise show.
        class FormattedSlider : public juce::Slider
        {
        public:
            std::function<juce::String(float)> valueToText;
            juce::String getTextFromValue(double value) override;
        };

        FormattedSlider slider_;
        juce::Label nameLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlowKnob)
    };

} // namespace pw8::plugin::ui
