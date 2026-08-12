#pragma once

#include <juce_core/juce_core.h>

// User favorites persisted outside shipped content (docs/PATCH_BROWSER.md phase 4).
namespace pw8::plugin::content
{
    class FavoritesStore
    {
    public:
        FavoritesStore();

        void load();
        void save();

        [[nodiscard]] bool isFavorite(const juce::String& absolutePath) const;
        void setFavorite(const juce::String& absolutePath, bool favorited);
        void toggleFavorite(const juce::String& absolutePath);

        [[nodiscard]] const juce::StringArray& paths() const noexcept { return paths_; }

    private:
        [[nodiscard]] static juce::File defaultStorageFile();

        juce::StringArray paths_;
    };

} // namespace pw8::plugin::content
