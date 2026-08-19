#pragma once

#include <functional>

#include <juce_core/juce_core.h>

// A real update-availability check, not a full auto-updater. Sparkle
// (the usual macOS answer) was deliberately not wired in: it expects an
// Xcode project (embeds a signed XPC Installer/Downloader helper bundle
// via build phases) and a continuous code-signing identity to trust
// replacing the running app -- this project builds via pure CMake and has
// no paid Developer ID cert yet (same blocker already tracked for
// notarization). Silently offering "auto-update" on top of that would be
// broken by construction. This instead does the one honest thing that's
// actually buildable today: check the real public GitHub Releases API and
// tell the user a newer version exists, same infra as
// net::MurmurApiClient. Never installs anything itself.
namespace pw8::plugin::net
{
    struct UpdateCheckResult
    {
        bool success = false;
        bool updateAvailable = false;
        juce::String currentVersion;
        juce::String latestVersion;
        juce::String releaseUrl;
    };

    class UpdateChecker
    {
    public:
        /// Fires once, on the message thread. `callback` is only informed
        /// of a real successful comparison -- a failed/unreachable check
        /// just reports success=false and is silently ignored by callers
        /// (never nags the user about a network hiccup).
        static void checkForUpdate(std::function<void(UpdateCheckResult)> callback);

        /// Exposed for testing -- compares two dotted version strings
        /// ("1.10.2" > "1.9.0"), tolerant of differing segment counts and
        /// a leading "v".
        [[nodiscard]] static bool isNewerVersion(const juce::String& latest, const juce::String& current);
    };

} // namespace pw8::plugin::net
