#include "content/PresetIndex.h"

#include "pw8/content/ContentPaths.hpp"

namespace pw8::plugin::content
{
    namespace
    {
        juce::String readMetadataString(const juce::var& meta, const char* key)
        {
            if (meta.isObject() && meta.hasProperty(key))
                return meta[key].toString();
            return {};
        }

        juce::StringArray readStringArray(const juce::var& meta, const char* key)
        {
            juce::StringArray out;
            if (!meta.isObject() || !meta.hasProperty(key))
                return out;
            const auto arr = meta[key];
            if (arr.isArray())
            {
                for (int i = 0; i < arr.size(); ++i)
                    out.add(arr[i].toString());
            }
            return out;
        }

        [[nodiscard]] bool isContextCrossoverMood(const juce::String& mood) noexcept
        {
            static const char* kContextTokens[] = {"cinematic", "score", "trailer", "game",      "worship",
                                                   "sleep",     "demo",  "ambient", "sound-design", nullptr};
            const auto lower = mood.trim().toLowerCase();
            for (std::size_t i = 0; kContextTokens[i] != nullptr; ++i)
            {
                if (lower == kContextTokens[i])
                    return true;
            }
            return false;
        }

        [[nodiscard]] juce::String displayDedupKey(const PresetEntry& entry)
        {
            return entry.category.trim().toLowerCase() + "|" + entry.name.trim().toLowerCase();
        }

        [[nodiscard]] juce::String summarizeEnginesFromPatchJson(const juce::var& parsed)
        {
            if (!parsed.isObject() || !parsed.hasProperty("layerA"))
                return "01";

            const auto layerA = parsed["layerA"];
            if (!layerA.isObject() || !layerA.hasProperty("operators"))
                return "01";

            const auto operators = layerA["operators"];
            if (!operators.isArray())
                return "01";

            juce::StringArray active;
            for (int i = 0; i < operators.size() && i < 8; ++i)
            {
                const auto op = operators[i];
                if (!op.isObject())
                    continue;

                const bool mixEnabled = op.hasProperty("mixEnabled") ? static_cast<bool>(op["mixEnabled"]) : true;
                const float level = static_cast<float>(op.getProperty("level", 0.0));
                if (mixEnabled && level > 0.01f)
                    active.add(juce::String(i + 1).paddedLeft('0', 2));
            }

            if (active.isEmpty())
                return "01";
            if (active.size() == 8)
                return "ALL 8";
            return active.joinIntoString(", ");
        }
    } // namespace

    void PresetIndex::rescan()
    {
        entries_.clear();
        juce::StringArray seenPaths;
        juce::StringArray seenDisplayKeys;

        for (const auto& rootStr : pw8::content::presetSearchRoots())
        {
            const juce::File root(rootStr);
            if (!root.isDirectory())
                continue;

            for (const auto& file : root.findChildFiles(juce::File::findFiles, true, "*.pw8"))
            {
                if (seenPaths.contains(file.getFullPathName()))
                    continue;

                auto entry = parsePresetFile(file);
                const auto displayKey = displayDedupKey(entry);
                if (seenDisplayKeys.contains(displayKey))
                    continue;

                seenPaths.add(file.getFullPathName());
                seenDisplayKeys.add(displayKey);
                entries_.add(std::move(entry));
            }
        }

        struct PathComparator
        {
            static int compareElements(const PresetEntry& a, const PresetEntry& b)
            {
                const int cat = a.category.compareIgnoreCase(b.category);
                if (cat != 0)
                    return cat;
                return a.name.compareIgnoreCase(b.name);
            }
        };
        PathComparator cmp;
        entries_.sort(cmp);
    }

    PresetEntry PresetIndex::parsePresetFile(const juce::File& file)
    {
        PresetEntry entry;
        entry.absolutePath = file.getFullPathName();

        const auto jsonText = file.loadFileAsString();
        const auto parsed = juce::JSON::parse(jsonText);
        if (parsed.isVoid())
        {
            entry.name = file.getFileNameWithoutExtension();
            entry.category = file.getParentDirectory().getFileName();
            return entry;
        }

        const auto meta = parsed.hasProperty("metadata") ? parsed["metadata"] : parsed;
        entry.name = readMetadataString(meta, "name");
        if (entry.name.isEmpty())
            entry.name = file.getFileNameWithoutExtension();
        entry.category = readMetadataString(meta, "category");
        if (entry.category.isEmpty())
            entry.category = file.getParentDirectory().getFileName();
        entry.description = readMetadataString(meta, "description");
        entry.author = readMetadataString(meta, "author");
        entry.moods = readStringArray(meta, "moods");
        entry.genres = readStringArray(meta, "genres");
        entry.tags = readStringArray(meta, "tags");
        entry.enginesSummary = summarizeEnginesFromPatchJson(parsed);
        return entry;
    }

    bool PresetIndex::arrayContainsIgnoreCase(const juce::StringArray& arr, const juce::String& value)
    {
        const auto needle = value.trim().toLowerCase();
        if (needle.isEmpty())
            return false;
        for (const auto& item : arr)
        {
            if (item.trim().toLowerCase() == needle)
                return true;
        }
        return false;
    }

    bool PresetIndex::entryMatchesFilter(const PresetEntry& entry, const PresetMetadataFilter& filter,
                                          const juce::StringArray* favoritePathsOnly)
    {
        if (favoritePathsOnly != nullptr && !favoritePathsOnly->contains(entry.absolutePath))
            return false;

        const auto cat = filter.category.trim().toLowerCase();
        if (cat.isNotEmpty() && entry.category.trim().toLowerCase() != cat)
            return false;

        const auto mood = filter.mood.trim().toLowerCase();
        if (mood.isNotEmpty() && !arrayContainsIgnoreCase(entry.moods, mood))
            return false;

        const auto genre = filter.genre.trim().toLowerCase();
        if (genre.isNotEmpty() && !arrayContainsIgnoreCase(entry.genres, genre)
            && !arrayContainsIgnoreCase(entry.moods, genre))
            return false;

        const auto tag = filter.tag.trim().toLowerCase();
        if (tag.isNotEmpty() && !arrayContainsIgnoreCase(entry.tags, tag))
            return false;

        const auto q = filter.query.trim().toLowerCase();
        if (q.isNotEmpty())
        {
            const auto haystack = (entry.name + " " + entry.description + " " + entry.category + " "
                                   + entry.moods.joinIntoString(" ") + " " + entry.genres.joinIntoString(" ") + " "
                                   + entry.tags.joinIntoString(" "))
                                      .toLowerCase();
            if (!haystack.contains(q))
                return false;
        }

        return true;
    }

    PresetMetadataFilter PresetIndex::filterWithFacetCleared(PresetMetadataFilter filter, PresetFacet facet)
    {
        switch (facet)
        {
            case PresetFacet::Category: filter.category = {}; break;
            case PresetFacet::Mood: filter.mood = {}; break;
            case PresetFacet::Genre: filter.genre = {}; break;
            case PresetFacet::Tag: filter.tag = {}; break;
        }
        return filter;
    }

    void PresetIndex::collectFacetValue(PresetFacet facet, const PresetEntry& entry, juce::StringArray& out)
    {
        auto addUnique = [&](const juce::String& value) {
            const auto trimmed = value.trim();
            if (trimmed.isNotEmpty() && !out.contains(trimmed, true))
                out.add(trimmed);
        };

        switch (facet)
        {
            case PresetFacet::Category:
                addUnique(entry.category);
                break;
            case PresetFacet::Mood:
                for (const auto& mood : entry.moods)
                    addUnique(mood);
                break;
            case PresetFacet::Genre:
                for (const auto& genre : entry.genres)
                    addUnique(genre);
                for (const auto& mood : entry.moods)
                {
                    if (isContextCrossoverMood(mood))
                        addUnique(mood);
                }
                break;
            case PresetFacet::Tag:
                for (const auto& tag : entry.tags)
                    addUnique(tag);
                break;
        }
    }

    juce::Array<PresetEntry> PresetIndex::filtered(const PresetMetadataFilter& filter,
                                                    const juce::StringArray* favoritePathsOnly) const
    {
        juce::Array<PresetEntry> out;
        const juce::StringArray* fav = filter.favoritesOnly ? favoritePathsOnly : nullptr;

        for (const auto& entry : entries_)
        {
            if (entryMatchesFilter(entry, filter, fav))
                out.add(entry);
        }
        return out;
    }

    juce::StringArray PresetIndex::uniqueFacetValues(PresetFacet facet, const PresetMetadataFilter& filter,
                                                      const juce::StringArray* favoritePathsOnly) const
    {
        const auto facetFilter = filterWithFacetCleared(filter, facet);
        const juce::StringArray* fav = facetFilter.favoritesOnly ? favoritePathsOnly : nullptr;

        juce::StringArray values;
        for (const auto& entry : entries_)
        {
            if (!entryMatchesFilter(entry, facetFilter, fav))
                continue;
            collectFacetValue(facet, entry, values);
        }
        values.sort(true);
        return values;
    }

    std::optional<PresetEntry> PresetIndex::nextAfter(const juce::String& currentPath,
                                                       const PresetMetadataFilter& filter,
                                                       const juce::StringArray* favoritePathsOnly) const
    {
        const auto list = filtered(filter, favoritePathsOnly);
        if (list.isEmpty())
            return std::nullopt;

        for (int i = 0; i < list.size(); ++i)
        {
            if (list[i].absolutePath == currentPath)
                return list[(i + 1) % list.size()];
        }
        return list[0];
    }

    std::optional<PresetEntry> PresetIndex::prevBefore(const juce::String& currentPath,
                                                        const PresetMetadataFilter& filter,
                                                        const juce::StringArray* favoritePathsOnly) const
    {
        const auto list = filtered(filter, favoritePathsOnly);
        if (list.isEmpty())
            return std::nullopt;

        for (int i = 0; i < list.size(); ++i)
        {
            if (list[i].absolutePath == currentPath)
                return list[(i - 1 + list.size()) % list.size()];
        }
        return list[list.size() - 1];
    }

    juce::StringArray PresetIndex::uniqueCategories() const
    {
        PresetMetadataFilter empty;
        return uniqueFacetValues(PresetFacet::Category, empty, nullptr);
    }

    juce::Array<PresetEntry> PresetIndex::filtered(const juce::String& query, const juce::String& category,
                                                    const juce::StringArray* favoritePathsOnly) const
    {
        PresetMetadataFilter filter;
        filter.query = query;
        filter.category = category;
        filter.favoritesOnly = favoritePathsOnly != nullptr;
        return filtered(filter, favoritePathsOnly);
    }

    std::optional<PresetEntry> PresetIndex::nextAfter(const juce::String& currentPath, const juce::String& query,
                                                       const juce::String& category,
                                                       const juce::StringArray* favoritePathsOnly) const
    {
        PresetMetadataFilter filter;
        filter.query = query;
        filter.category = category;
        filter.favoritesOnly = favoritePathsOnly != nullptr;
        return nextAfter(currentPath, filter, favoritePathsOnly);
    }

    std::optional<PresetEntry> PresetIndex::prevBefore(const juce::String& currentPath, const juce::String& query,
                                                        const juce::String& category,
                                                        const juce::StringArray* favoritePathsOnly) const
    {
        PresetMetadataFilter filter;
        filter.query = query;
        filter.category = category;
        filter.favoritesOnly = favoritePathsOnly != nullptr;
        return prevBefore(currentPath, filter, favoritePathsOnly);
    }

} // namespace pw8::plugin::content
