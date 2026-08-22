#pragma once

#include <array>
#include <cstddef>

#include <juce_audio_processors/juce_audio_processors.h>

namespace pw8::fathom
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

    /// Real structural twin of quasar_plugin's own ParamFieldSpec table.
    /// Every Algorithmic-mode field name/range/default mirrors the real,
    /// already-shipping `pw8::effects::EffectSlotParams` reverb fields
    /// exactly (engine/include/pw8/effects/EffectTypes.hpp's own doc
    /// comments are the source for every range here) -- Fathom doesn't
    /// invent a second reverb parameter model, it exposes the real one.
    inline constexpr std::size_t kNumFathomParams = 24;

    extern const std::array<ParamFieldSpec, kNumFathomParams> kFathomParamSpecs;

    [[nodiscard]] juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace pw8::fathom
