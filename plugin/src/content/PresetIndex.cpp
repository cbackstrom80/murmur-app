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
    } // namespace

    void PresetIndex::rescan()
    {
        entries_.clear();
        juce::StringArray seenPaths;

        for (const auto& rootStr : pw8::content::presetSearchRoots())
        {
            const juce::File root(rootStr);
            if (!root.isDirectory())
                continue;

            for (const auto& file : root.findChildFiles(juce::File::findFiles, true, "*.pw8"))
            {
                if (seenPaths.contains(file.getFullPathName()))
                    continue;
                seenPaths.add(file.getFullPathName());
                entries_.add(parsePresetFile(file));
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
        entry.moods = readStringArray(meta, "moods");
        entry.tags = readStringArray(meta, "tags");
        return entry;
    }

    juce::Array<PresetEntry> PresetIndex::filteredCopy(const juce::String& query, const juce::String& category,
                                                        const juce::StringArray* favoritePathsOnly) const
    {
        juce::Array<PresetEntry> out;
        const auto q = query.trim().toLowerCase();
        const auto cat = category.trim().toLowerCase();
        const bool favoritesOnly = favoritePathsOnly != nullptr;

        for (const auto& e : entries_)
        {
            if (favoritesOnly && !favoritePathsOnly->contains(e.absolutePath))
                continue;
            if (!favoritesOnly && cat.isNotEmpty() && !e.category.toLowerCase().contains(cat))
                continue;
            if (q.isNotEmpty())
            {
                const auto haystack = (e.name + " " + e.description + " " + e.category + " " +
                                       e.moods.joinIntoString(" ") + " " + e.tags.joinIntoString(" "))
                                          .toLowerCase();
                if (!haystack.contains(q))
                    continue;
            }
            out.add(e);
        }
        return out;
    }

    juce::Array<PresetEntry> PresetIndex::filtered(const juce::String& query, const juce::String& category,
                                                    const juce::StringArray* favoritePathsOnly) const
    {
        return filteredCopy(query, category, favoritePathsOnly);
    }

    std::optional<PresetEntry> PresetIndex::nextAfter(const juce::String& currentPath, const juce::String& query,
                                                       const juce::String& category,
                                                       const juce::StringArray* favoritePathsOnly) const
    {
        const auto list = filteredCopy(query, category, favoritePathsOnly);
        if (list.isEmpty())
            return std::nullopt;

        for (int i = 0; i < list.size(); ++i)
        {
            if (list[i].absolutePath == currentPath)
                return list[(i + 1) % list.size()];
        }
        return list[0];
    }

    std::optional<PresetEntry> PresetIndex::prevBefore(const juce::String& currentPath, const juce::String& query,
                                                        const juce::String& category,
                                                        const juce::StringArray* favoritePathsOnly) const
    {
        const auto list = filteredCopy(query, category, favoritePathsOnly);
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
        juce::StringArray cats;
        for (const auto& e : entries_)
        {
            if (e.category.isNotEmpty() && !cats.contains(e.category, true))
                cats.add(e.category);
        }
        cats.sort(true);
        return cats;
    }

} // namespace pw8::plugin::content
