#pragma once

#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>

namespace pw8::quasar
{
    inline constexpr int kQuasarPresetSchemaVersion = 1;

    [[nodiscard]] std::optional<juce::String> exportPresetJson(const juce::AudioProcessorValueTreeState& apvts);
    [[nodiscard]] bool importPresetJson(juce::AudioProcessorValueTreeState& apvts, const juce::String& jsonText);

    [[nodiscard]] juce::File factoryPresetsDirectory();
    [[nodiscard]] bool savePresetFile(const juce::File& file, const juce::AudioProcessorValueTreeState& apvts,
                                      const juce::String& name, const juce::String& author = "MURMUR Sound Design",
                                      const juce::String& description = {});
    [[nodiscard]] bool loadPresetFile(const juce::File& file, juce::AudioProcessorValueTreeState& apvts);

} // namespace pw8::quasar
