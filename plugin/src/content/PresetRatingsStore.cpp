#include "content/PresetRatingsStore.h"

namespace pw8::plugin::content
{
    PresetRatingsStore::PresetRatingsStore()
    {
        load();
    }

    juce::File PresetRatingsStore::defaultStorageFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("MURMUR/ratings.json");
    }

    void PresetRatingsStore::load()
    {
        ratingsByPath_.clear();
        const juce::File file = defaultStorageFile();
        if (!file.existsAsFile())
            return;

        const auto parsed = juce::JSON::parse(file.loadFileAsString());
        if (!parsed.isObject())
            return;

        if (auto* obj = parsed.getDynamicObject())
        {
            for (const auto& prop : obj->getProperties())
            {
                const auto path = prop.name.toString();
                const int stars = static_cast<int>(prop.value);
                if (path.isNotEmpty() && stars >= 1 && stars <= 5)
                    ratingsByPath_.set(path, stars);
            }
        }
    }

    void PresetRatingsStore::save()
    {
        const juce::File file = defaultStorageFile();
        file.getParentDirectory().createDirectory();

        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        for (auto it = ratingsByPath_.begin(); it != ratingsByPath_.end(); ++it)
            root->setProperty(it.getKey(), it.getValue());

        file.replaceWithText(juce::JSON::toString(juce::var(root.get()), true));
    }

    int PresetRatingsStore::ratingForPath(const juce::String& absolutePath) const
    {
        if (absolutePath.isEmpty())
            return 0;
        return ratingsByPath_.contains(absolutePath) ? ratingsByPath_[absolutePath] : 0;
    }

    void PresetRatingsStore::setRating(const juce::String& absolutePath, int stars)
    {
        if (absolutePath.isEmpty())
            return;

        stars = juce::jlimit(0, 5, stars);
        const int previous = ratingForPath(absolutePath);
        if (stars <= 0)
            ratingsByPath_.remove(absolutePath);
        else
            ratingsByPath_.set(absolutePath, stars);

        if (previous != stars)
            save();
    }

    void PresetRatingsStore::clearRating(const juce::String& absolutePath)
    {
        setRating(absolutePath, 0);
    }

} // namespace pw8::plugin::content
