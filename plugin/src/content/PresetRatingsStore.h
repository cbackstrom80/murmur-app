#pragma once

#include <juce_core/juce_core.h>

namespace pw8::plugin::content
{
    /// User star ratings (1–5) persisted outside shipped preset content.
    class PresetRatingsStore
    {
    public:
        PresetRatingsStore();

        void load();
        void save();

        /// @return 0 if unrated, otherwise 1–5.
        [[nodiscard]] int ratingForPath(const juce::String& absolutePath) const;
        void setRating(const juce::String& absolutePath, int stars);
        void clearRating(const juce::String& absolutePath);

    private:
        [[nodiscard]] static juce::File defaultStorageFile();

        juce::HashMap<juce::String, int> ratingsByPath_;
    };

} // namespace pw8::plugin::content
