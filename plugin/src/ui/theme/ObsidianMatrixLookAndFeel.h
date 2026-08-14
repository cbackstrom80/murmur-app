#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// Mod-matrix row chrome: glass groove bipolar slider, glass combos, compact bypass toggle.
    class ObsidianMatrixLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        ObsidianMatrixLookAndFeel();

        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                              float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
                              juce::Slider& slider) override;

        void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                          int buttonW, int buttonH, juce::ComboBox& box) override;

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

        juce::Font getComboBoxFont(juce::ComboBox& box) override;
    };

} // namespace pw8::plugin::ui
