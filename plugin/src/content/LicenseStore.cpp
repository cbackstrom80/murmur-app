#include "content/LicenseStore.h"

namespace pw8::plugin::content
{
    LicenseStore::LicenseStore() { load(); }

    juce::File LicenseStore::storageFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("MURMUR/license.json");
    }

    void LicenseStore::load()
    {
        info_ = {};
        const juce::File file = storageFile();
        if (!file.existsAsFile())
            return;

        const auto parsed = juce::JSON::parse(file.loadFileAsString());
        if (!parsed.isObject())
            return;

        info_.licenseKey = parsed.getProperty("licenseKey", "").toString();
        info_.displayName = parsed.getProperty("displayName", "").toString();
        info_.email = parsed.getProperty("email", "").toString();
        info_.entitled = static_cast<bool>(parsed.getProperty("entitled", false));
        info_.activatedAt = parsed.getProperty("activatedAt", "").toString();
        info_.isDevCurator = static_cast<bool>(parsed.getProperty("isDevCurator", false));
    }

    void LicenseStore::save()
    {
        const juce::File file = storageFile();
        file.getParentDirectory().createDirectory();

        auto* obj = new juce::DynamicObject();
        obj->setProperty("licenseKey", info_.licenseKey);
        obj->setProperty("displayName", info_.displayName);
        obj->setProperty("email", info_.email);
        obj->setProperty("entitled", info_.entitled);
        obj->setProperty("activatedAt", info_.activatedAt);
        obj->setProperty("isDevCurator", info_.isDevCurator);

        file.replaceWithText(juce::JSON::toString(juce::var(obj), true));
    }

    void LicenseStore::clear()
    {
        info_ = {};
        storageFile().deleteFile();
    }

    void LicenseStore::setInfo(LicenseInfo info)
    {
        info_ = std::move(info);
        save();
    }

} // namespace pw8::plugin::content
