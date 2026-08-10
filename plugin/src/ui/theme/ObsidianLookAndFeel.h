#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// The single place that paints every control in the OBSIDIAN skin. Components
// under plugin/src/ui/ never draw their own knob/toggle chrome -- they set a
// juce::Slider/juce::ToggleButton's behavior (range, style) and let this
// LookAndFeel render it, so the visual language stays consistent by construction
// rather than by convention every component has to remember to follow.
namespace pw8::plugin::ui
{
    class ObsidianLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        ObsidianLookAndFeel();

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

        juce::Font getLabelFont(juce::Label& label) override;
    };

} // namespace pw8::plugin::ui
