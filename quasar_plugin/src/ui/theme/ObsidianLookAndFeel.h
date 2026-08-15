#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::quasar::ui
{
    class ObsidianLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        ObsidianLookAndFeel();

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional,
                               float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

        void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

        juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

        void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override;

        void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override;

        void drawLabel(juce::Graphics& g, juce::Label& label) override;

        int getDefaultScrollbarWidth() override;

        void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
                           bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver,
                           bool isMouseDown) override;

        juce::Font getLabelFont(juce::Label& label) override;

        juce::Font getPopupMenuFont() override;

        juce::Font getComboBoxFont(juce::ComboBox& box) override;
    };

} // namespace pw8::quasar::ui
