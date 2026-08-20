#pragma once

#include <juce_core/juce_core.h>

// Persisted result of a successful ACTIVATE LICENSE call against
// murmur-web's /api/v1/activate — see KeyActivationOverlay and
// net/MurmurApiClient. Same load/save pattern as FavoritesStore/
// PresetRatingsStore, not a new persistence mechanism. Real path is
// ~/Library/MURMUR/license.json, not "Application Support" -- verified
// directly (stderr trace) while debugging curator-mode: JUCE's
// userApplicationDataDirectory resolves to plain ~/Library on macOS, not
// ~/Library/Application Support, despite what this comment (and the
// sibling stores' own comments) previously assumed. Functionally
// harmless -- every real read/write in this codebase goes through the
// same getSpecialLocation() call, so it's self-consistent -- but matters
// if you're ever hand-editing this file for a test, as I was.
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
        bool isDevCurator = false; // real -- unlocks CuratorReviewOverlay. See
                                    // /api/v1/activate's own isCurator field.

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
