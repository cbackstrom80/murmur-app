#include "content/FavoritesStore.h"

namespace pw8::plugin::content
{
    FavoritesStore::FavoritesStore()
    {
        load();
    }

    juce::File FavoritesStore::defaultStorageFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Patchwork Eight/favorites.json");
    }

    void FavoritesStore::load()
    {
        paths_.clear();
        const juce::File file = defaultStorageFile();
        if (!file.existsAsFile())
            return;

        const auto parsed = juce::JSON::parse(file.loadFileAsString());
        if (parsed.isArray())
        {
            for (int i = 0; i < parsed.size(); ++i)
            {
                const auto path = parsed[i].toString();
                if (path.isNotEmpty() && !paths_.contains(path))
                    paths_.add(path);
            }
        }
    }

    void FavoritesStore::save()
    {
        const juce::File file = defaultStorageFile();
        file.getParentDirectory().createDirectory();

        juce::Array<juce::var> arr;
        for (const auto& path : paths_)
            arr.add(path);

        const juce::var root(arr);
        file.replaceWithText(juce::JSON::toString(root, true));
    }

    bool FavoritesStore::isFavorite(const juce::String& absolutePath) const
    {
        return paths_.contains(absolutePath);
    }

    void FavoritesStore::setFavorite(const juce::String& absolutePath, bool favorited)
    {
        if (absolutePath.isEmpty())
            return;

        const bool had = paths_.contains(absolutePath);
        if (favorited && !had)
            paths_.add(absolutePath);
        else if (!favorited && had)
            paths_.removeString(absolutePath);

        if (had != favorited)
            save();
    }

    void FavoritesStore::toggleFavorite(const juce::String& absolutePath)
    {
        setFavorite(absolutePath, !isFavorite(absolutePath));
    }

} // namespace pw8::plugin::content
