#include "DualKnobLookAndFeel.h"

#include "DeckedKnobDraw.h"
#include "ObsidianPalette.h"
#include "ObsidianRotary.h"

namespace pw8::plugin::ui
{
    void DualKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPosProportional, float rotaryStartAngle,
                                                float rotaryEndAngle, juce::Slider& slider)
    {
        if (width <= 0 || height <= 0)
            return;

        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                     static_cast<float>(width), static_cast<float>(height))
                                 .reduced(4.0f);
        const float maxDial =
            static_cast<float>(slider.getProperties().getWithDefault("maxDialDiameter", 88));
        const float diameter =
            juce::jmin(maxDial, juce::jmax(16.0f, juce::jmin(bounds.getWidth(), bounds.getHeight())));
        const auto knobBounds = bounds.withSizeKeepingCentre(diameter, diameter);
        const float proportional = rotary::normalisedProportional(sliderPosProportional, slider);
        const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
        const bool active = slider.isMouseOverOrDragging();

        if (knobType_ == KnobType::Outer)
        {
            decked::drawDualOuterRotarySlider(g, knobBounds, proportional, rotaryStartAngle, rotaryEndAngle, active);
            return;
        }

        decked::drawDualInnerRotarySlider(g, knobBounds, proportional, rotaryStartAngle, rotaryEndAngle, accent,
                                          active);
    }

} // namespace pw8::plugin::ui
