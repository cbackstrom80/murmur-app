#pragma once

#include <cmath>

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

namespace pw8::quasar::ui::rotary
{
    /// Shared Murmur rotary sweep — must match GlowKnob / ConcentricGlowKnob attachments.
    inline constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
    inline constexpr float kEndAngle = juce::MathConstants<float>::pi * 2.8f;
    inline constexpr float kSweep = kEndAngle - kStartAngle;

    /// Unit direction for a JUCE rotary angle (0 = 12 o'clock, clockwise positive).
    /// Must match Path::addCentredArc / Slider::handleRotaryDrag — not std::cos/sin.
    [[nodiscard]] inline juce::Point<float> unitDirectionAtAngle(float angleRadians) noexcept
    {
        return {std::sin(angleRadians), -std::cos(angleRadians)};
    }

    [[nodiscard]] inline float proportionalToAngle(float proportional, float rotaryStartAngle,
                                                 float rotaryEndAngle) noexcept
    {
        return rotaryStartAngle
               + juce::jlimit(0.0f, 1.0f, proportional) * (rotaryEndAngle - rotaryStartAngle);
    }

    [[nodiscard]] inline float proportionalToAngle(float proportional) noexcept
    {
        return proportionalToAngle(proportional, kStartAngle, kEndAngle);
    }

    [[nodiscard]] inline float normalisedProportional(float sliderPosProportional,
                                                      const juce::Slider& slider) noexcept
    {
        juce::ignoreUnused(slider);
        float proportional = juce::jlimit(0.0f, 1.0f, sliderPosProportional);
        if (proportional >= 1.0f - 1.0e-4f)
            proportional = 1.0f;
        else if (proportional <= 1.0e-4f)
            proportional = 0.0f;
        return proportional;
    }

} // namespace pw8::quasar::ui::rotary
