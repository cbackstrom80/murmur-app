#pragma once

#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

namespace pw8::fathom
{
    inline constexpr int kFathomPresetSchemaVersion = 1;

    [[nodiscard]] std::optional<juce::String> exportPresetJson(const juce::AudioProcessorValueTreeState& apvts);
    [[nodiscard]] bool importPresetJson(juce::AudioProcessorValueTreeState& apvts, const juce::String& jsonText);

} // namespace pw8::fathom
