#pragma once

#include <juce_core/juce_core.h>

// Persisted result of a successful ACTIVATE LICENSE call against
// murmur-web's /api/v1/activate — see KeyActivationOverlay and
// net/MurmurApiClient. Same load/save-to-Application-Support pattern as
// FavoritesStore/PresetRatingsStore, not a new persistence mechanism.
namespace pw8::plugin::content
{
    struct LicenseInfo
    {
        juce::String licenseKey;   // raw key, kept locally so future syncs don't
                                    // require re-typing it -- murmur-web only ever
                                    // stores its hash, so this is the one copy of
                                    // record on the user's own machine.
        juce::String displayName;
        juce::String email;
        bool entitled = false;     // real Entitlement state as of last activation --
                                    // not enforced against features yet (Stripe isn't
                                    // wired on the backend), purely informational.
        juce::String activatedAt;  // ISO8601, local clock, informational only.

        [[nodiscard]] bool isActivated() const noexcept { return licenseKey.isNotEmpty(); }
    };

    class LicenseStore
    {
    public:
        LicenseStore();

        void load();
        void save();
        void clear();

        [[nodiscard]] const LicenseInfo& info() const noexcept { return info_; }
        void setInfo(LicenseInfo info);

    private:
        [[nodiscard]] static juce::File storageFile();

        LicenseInfo info_;
    };

} // namespace pw8::plugin::content
