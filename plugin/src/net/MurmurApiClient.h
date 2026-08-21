#pragma once

#include <functional>

#include <juce_core/juce_core.h>

// First real network integration in the plugin: talks to murmur-web's
// /api/v1 endpoints (see murmur-web's src/app/api/v1/{activate,library})
// to back the ACTIVATE LICENSE flow (KeyActivationOverlay). Every call
// runs on a short-lived background thread (juce::Thread::launch -- JUCE
// has no built-in non-blocking URL fetch) and delivers its result back on
// the message thread, so UI callers never touch threading directly.
namespace pw8::plugin::net
{
    struct ActivateResult
    {
        bool success = false;
        juce::String errorMessage; // set when !success -- either a transport
                                    // failure or the server's own {error} body
        juce::String displayName;
        juce::String email;
        bool entitled = false;
        juce::String keyName;
        bool isCurator = false; // real -- see /api/v1/activate's own response field
    };

    struct LibraryPatch
    {
        juce::String slug;
        juce::String name;
        juce::String category;
        juce::String downloadUrl;
        /// Real, per-request field from /api/v1/library -- true when this
        /// patch's author is the account the activated license key belongs
        /// to. Still the same community-wide synced set; this just marks
        /// which of those are the caller's own, so the plugin can route them
        /// into the local "User" preset bank instead of "Community".
        bool isOwn = false;
    };

    struct LibraryResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::Array<LibraryPatch> patches;
    };

    /// One row in the curator review queue -- metadata only, no audio
    /// preview (real limitation, see CuratorReviewOverlay's own doc
    /// comment for why).
    struct CuratorQueuePatch
    {
        juce::String slug;
        juce::String name;
        juce::String category;
        juce::StringArray tags;
        juce::String source; // "manual" | "ai_generated"
    };

    struct CuratorQueueResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::Array<CuratorQueuePatch> patches;
    };

    struct CuratorVoteResult
    {
        bool success = false;
        juce::String errorMessage;
        bool removed = false;
    };

    class MurmurApiClient
    {
    public:
        /// The real deployed murmur-web origin (docs/DEPLOY.md) -- HTTP, not
        /// HTTPS, since that's the honest current state of the deployment
        /// (no domain/TLS in front of it yet). Override with the
        /// MURMUR_WEB_API_BASE environment variable for local dev against
        /// `npx next dev`.
        [[nodiscard]] static juce::String defaultBaseUrl();

        /// POSTs to /api/v1/activate. `callback` fires once, on the message
        /// thread, whether the call succeeds or fails.
        static void activate(const juce::String& licenseKey, std::function<void(ActivateResult)> callback);

        /// GETs /api/v1/library with the key as a Bearer token. `callback`
        /// fires once, on the message thread.
        static void fetchLibrary(const juce::String& licenseKey, std::function<void(LibraryResult)> callback);

        /// GETs /api/v1/curator-queue. Bearer auth, server-side isCurator-gated
        /// (a non-curator key gets a real 403, surfaced as errorMessage).
        static void fetchCuratorQueue(const juce::String& licenseKey, std::function<void(CuratorQueueResult)> callback);

        /// POSTs /api/v1/patches/[slug]/curate with {"vote": "up"|"down"}.
        static void submitCuratorVote(const juce::String& licenseKey, const juce::String& slug, bool voteUp,
                                       std::function<void(CuratorVoteResult)> callback);
    };

} // namespace pw8::plugin::net
