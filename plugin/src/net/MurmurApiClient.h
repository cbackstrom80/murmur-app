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
    };

    struct LibraryPatch
    {
        juce::String slug;
        juce::String name;
        juce::String category;
        juce::String downloadUrl;
    };

    struct LibraryResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::Array<LibraryPatch> patches;
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
    };

} // namespace pw8::plugin::net
