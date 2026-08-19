#pragma once

#include <functional>
#include <optional>

#include <juce_core/juce_core.h>

namespace pw8::plugin::content
{
    struct PresetEntry
    {
        juce::String absolutePath;
        juce::String name;
        juce::String category;
        juce::String description;
        juce::String author;
        juce::StringArray moods;
        juce::StringArray genres;
        juce::StringArray tags;
        /// Comma-separated active engine indices (e.g. `"01, 02, 04"` or `"ALL 8"`).
        juce::String enginesSummary;
    };

    /// Multi-facet preset browser filter — AND across dimensions (docs/PATCH_BROWSER.md Phase 4).
    struct PresetMetadataFilter
    {
        juce::String query;
        juce::String category;
        juce::String mood;
        juce::String genre; ///< Context / use-case; also matches legacy values stored in moods[].
        juce::String tag;
        bool favoritesOnly = false;

        [[nodiscard]] bool isNarrowed() const noexcept
        {
            return favoritesOnly || query.trim().isNotEmpty() || category.trim().isNotEmpty()
                   || mood.trim().isNotEmpty() || genre.trim().isNotEmpty() || tag.trim().isNotEmpty();
        }
    };

    /// Alias for progressive funnel filtering in the preset browser overlay.
    using FunnelState = PresetMetadataFilter;

    enum class PresetFacet : std::uint8_t
    {
        Category = 0,
        Mood,
        Genre,
        Tag
    };

    /// Scans preset directories once; metadata-only parse (no full PatchSerializer).
    class PresetIndex
    {
    public:
        void rescan();

        [[nodiscard]] const juce::Array<PresetEntry>& allEntries() const noexcept { return entries_; }

        [[nodiscard]] juce::Array<PresetEntry> filtered(const PresetMetadataFilter& filter,
                                                         const juce::StringArray* favoritePathsOnly = nullptr) const;

        [[nodiscard]] std::optional<PresetEntry> nextAfter(const juce::String& currentPath,
                                                            const PresetMetadataFilter& filter = {},
                                                            const juce::StringArray* favoritePathsOnly = nullptr) const;
        [[nodiscard]] std::optional<PresetEntry> prevBefore(const juce::String& currentPath,
                                                             const PresetMetadataFilter& filter = {},
                                                             const juce::StringArray* favoritePathsOnly = nullptr) const;

        using EntryPredicate = std::function<bool(const PresetEntry&)>;

        /// Values still available for one facet given the other active filters (faceted search).
        [[nodiscard]] juce::StringArray uniqueFacetValues(PresetFacet facet,
                                                           const PresetMetadataFilter& filter,
                                                           const juce::StringArray* favoritePathsOnly = nullptr,
                                                           const EntryPredicate& entryPredicate = {}) const;

        [[nodiscard]] juce::StringArray uniqueCategories() const;

        // Legacy convenience — maps to PresetMetadataFilter with category only.
        [[nodiscard]] juce::Array<PresetEntry> filtered(const juce::String& query, const juce::String& category,
                                                         const juce::StringArray* favoritePathsOnly = nullptr) const;
        [[nodiscard]] std::optional<PresetEntry> nextAfter(const juce::String& currentPath, const juce::String& query,
                                                            const juce::String& category,
                                                            const juce::StringArray* favoritePathsOnly = nullptr) const;
        [[nodiscard]] std::optional<PresetEntry> prevBefore(const juce::String& currentPath, const juce::String& query,
                                                             const juce::String& category,
                                                             const juce::StringArray* favoritePathsOnly = nullptr) const;

    private:
        [[nodiscard]] static PresetEntry parsePresetFile(const juce::File& file);
        [[nodiscard]] static bool entryMatchesFilter(const PresetEntry& entry, const PresetMetadataFilter& filter,
                                                      const juce::StringArray* favoritePathsOnly);
        [[nodiscard]] static PresetMetadataFilter filterWithFacetCleared(PresetMetadataFilter filter,
                                                                          PresetFacet facet);
        static void collectFacetValue(PresetFacet facet, const PresetEntry& entry,
                                                     juce::StringArray& out);
        [[nodiscard]] static bool arrayContainsIgnoreCase(const juce::StringArray& arr, const juce::String& value);

        juce::Array<PresetEntry> entries_;
    };

} // namespace pw8::plugin::content
