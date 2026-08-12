#include "content/WavetableIndex.h"

#include "pw8/content/ContentPaths.hpp"

namespace pw8::plugin::content
{
    namespace
    {
        [[nodiscard]] juce::String resolveToAbsolute(const juce::String& pathOrId)
        {
            if (pathOrId.isEmpty())
                return {};
            const juce::File asFile(pathOrId);
            if (asFile.existsAsFile())
                return asFile.getFullPathName();
            if (const auto resolved = pw8::content::resolveWavetablePath(pathOrId.toStdString()))
                return juce::String(*resolved);
            return pathOrId;
        }
    } // namespace

    void WavetableIndex::rescan()
    {
        entries_.clear();
        juce::StringArray seenPaths;

        for (const auto& rootStr : pw8::content::wavetableSearchRoots())
        {
            const juce::File root(rootStr);
            if (!root.isDirectory())
                continue;

            for (const auto& file : root.findChildFiles(juce::File::findFiles, false, "*.json"))
            {
                if (seenPaths.contains(file.getFullPathName()))
                    continue;
                seenPaths.add(file.getFullPathName());
                entries_.add(entryFromFile(file));
            }
        }

        struct NameComparator
        {
            static int compareElements(const WavetableEntry& a, const WavetableEntry& b)
            {
                return a.name.compareIgnoreCase(b.name);
            }
        };
        NameComparator cmp;
        entries_.sort(cmp);
    }

    WavetableEntry WavetableIndex::entryFromFile(const juce::File& file)
    {
        WavetableEntry entry;
        entry.absolutePath = file.getFullPathName();
        entry.name = file.getFileNameWithoutExtension().replaceCharacter('-', ' ').toUpperCase();
        return entry;
    }

    std::optional<WavetableEntry> WavetableIndex::entryForPathOrId(const juce::String& pathOrId) const
    {
        const auto abs = resolveToAbsolute(pathOrId);
        for (const auto& e : entries_)
        {
            if (e.absolutePath == abs || e.absolutePath == pathOrId)
                return e;
        }
        return std::nullopt;
    }

    int WavetableIndex::indexOf(const juce::String& currentPathOrId) const
    {
        const auto abs = resolveToAbsolute(currentPathOrId);
        for (int i = 0; i < entries_.size(); ++i)
        {
            if (entries_[i].absolutePath == abs || entries_[i].absolutePath == currentPathOrId)
                return i;
        }
        return -1;
    }

    std::optional<WavetableEntry> WavetableIndex::nextAfter(const juce::String& currentPathOrId) const
    {
        if (entries_.isEmpty())
            return std::nullopt;

        const int idx = indexOf(currentPathOrId);
        if (idx >= 0)
            return entries_[(idx + 1) % entries_.size()];
        return entries_[0];
    }

    std::optional<WavetableEntry> WavetableIndex::prevBefore(const juce::String& currentPathOrId) const
    {
        if (entries_.isEmpty())
            return std::nullopt;

        const int idx = indexOf(currentPathOrId);
        if (idx >= 0)
            return entries_[(idx - 1 + entries_.size()) % entries_.size()];
        return entries_[entries_.size() - 1];
    }

} // namespace pw8::plugin::content
