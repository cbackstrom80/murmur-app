#pragma once

#include <optional>

#include <juce_core/juce_core.h>

namespace pw8::plugin::content
{
    struct WavetableEntry
    {
        juce::String absolutePath;
        juce::String name;
    };

    /// Scans content/wavetables/*.json under all registered content roots.
    class WavetableIndex
    {
    public:
        void rescan();

        [[nodiscard]] const juce::Array<WavetableEntry>& allEntries() const noexcept { return entries_; }

        [[nodiscard]] std::optional<WavetableEntry> entryForPathOrId(const juce::String& pathOrId) const;

        [[nodiscard]] std::optional<WavetableEntry> nextAfter(const juce::String& currentPathOrId) const;
        [[nodiscard]] std::optional<WavetableEntry> prevBefore(const juce::String& currentPathOrId) const;

        [[nodiscard]] int indexOf(const juce::String& currentPathOrId) const;

    private:
        [[nodiscard]] static WavetableEntry entryFromFile(const juce::File& file);

        juce::Array<WavetableEntry> entries_;
    };

} // namespace pw8::plugin::content
