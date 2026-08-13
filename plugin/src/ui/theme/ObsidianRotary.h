#pragma once

#include <juce_core/juce_core.h>

namespace pw8::plugin::ui::rotary
{
    /// Shared Murmur rotary sweep — must match GlowKnob / ConcentricGlowKnob attachments.
    inline constexpr float kStartAngle = juce::MathConstants<float>::pi * 1.2f;
    inline constexpr float kEndAngle = juce::MathConstants<float>::pi * 2.8f;
    inline constexpr float kSweep = kEndAngle - kStartAngle;

    [[nodiscard]] inline float proportionalToAngle(float proportional) noexcept
    {
        return kStartAngle + juce::jlimit(0.0f, 1.0f, proportional) * kSweep;
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

} // namespace pw8::plugin::ui::rotary
