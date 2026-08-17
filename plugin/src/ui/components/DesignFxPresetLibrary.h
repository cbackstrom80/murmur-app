#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace pw8::plugin::ui
{
    class DesignFxUiState;

    struct DesignFxPresetEntry
    {
        std::size_t chipIndex = 0;
        juce::String name;
        juce::String modePill;
        juce::File sourceFile;
        std::vector<std::pair<juce::String, float>> floatParams;
        std::vector<std::pair<juce::String, int>> intParams;
        std::array<float, 6> uiKnobs{};
        bool hasUiKnobs = false;
    };

    /// Loads chip-scoped FX tweak presets from `content/design-fx/*.json`.
    class DesignFxPresetLibrary
    {
    public:
        void rescan();

        [[nodiscard]] std::size_t countForChip(std::size_t chipIndex) const;
        [[nodiscard]] const DesignFxPresetEntry& entry(std::size_t chipIndex, std::size_t presetIndex) const;
        [[nodiscard]] bool hasEntriesForChip(std::size_t chipIndex) const;

        void applyEntry(const DesignFxPresetEntry& preset, juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& paramPrefix, DesignFxUiState* uiState = nullptr) const;

        [[nodiscard]] DesignFxPresetEntry captureEntry(std::size_t chipIndex,
                                                       juce::AudioProcessorValueTreeState& apvts,
                                                       const juce::String& paramPrefix,
                                                       const DesignFxUiState& uiState,
                                                       const juce::String& modePill,
                                                       const juce::String& name) const;

        [[nodiscard]] bool saveEntry(const DesignFxPresetEntry& entry) const;

        [[nodiscard]] bool isUserPreset(const DesignFxPresetEntry& entry) const;

        [[nodiscard]] bool deleteEntry(std::size_t chipIndex, std::size_t presetIndex) const;

        [[nodiscard]] bool renameEntry(std::size_t chipIndex, std::size_t presetIndex, const juce::String& newName) const;

    private:
        [[nodiscard]] bool loadFile(const juce::File& file);
        void mergePw8Sidecar(DesignFxPresetEntry& entry, const juce::var& parsed) const;

        std::array<std::vector<DesignFxPresetEntry>, 12> byChip_;
    };

} // namespace pw8::plugin::ui
