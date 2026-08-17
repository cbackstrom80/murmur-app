#pragma once

#include <array>
#include <cstddef>

#include <juce_audio_processors/juce_audio_processors.h>

namespace pw8::quasar
{
    struct ParamFieldSpec
    {
        const char* id;
        const char* label;
        float minValue;
        float maxValue;
        float defaultValue;
        bool discrete = false;
    };

    inline constexpr std::size_t kNumQuasarParams = 28;

    extern const std::array<ParamFieldSpec, kNumQuasarParams> kQuasarParamSpecs;

    [[nodiscard]] juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace pw8::quasar
