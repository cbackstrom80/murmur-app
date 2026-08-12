#pragma once

#include <optional>
#include <string>
#include <vector>

#include <juce_core/juce_core.h>

namespace pw8::plugin::content
{
    struct PresetEntry
    {
        juce::String absolutePath;
        juce::String name;
        juce::String category;
        juce::String description;
        juce::StringArray moods;
        juce::StringArray tags;
    };

    /// Scans preset directories once; metadata-only parse (no full PatchSerializer).
    class PresetIndex
    {
    public:
        void rescan();

        [[nodiscard]] const juce::Array<PresetEntry>& allEntries() const noexcept { return entries_; }

        [[nodiscard]] juce::Array<PresetEntry> filtered(const juce::String& query,
                                                         const juce::String& category) const;

        [[nodiscard]] std::optional<PresetEntry> nextAfter(const juce::String& currentPath,
                                                            const juce::String& query = {},
                                                            const juce::String& category = {}) const;
        [[nodiscard]] std::optional<PresetEntry> prevBefore(const juce::String& currentPath,
                                                             const juce::String& query = {},
                                                             const juce::String& category = {}) const;

        [[nodiscard]] juce::StringArray uniqueCategories() const;

    private:
        [[nodiscard]] static PresetEntry parsePresetFile(const juce::File& file);
        [[nodiscard]] juce::Array<PresetEntry> filteredCopy(const juce::String& query,
                                                             const juce::String& category) const;

        juce::Array<PresetEntry> entries_;
    };

} // namespace pw8::plugin::content
