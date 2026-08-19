#include "net/MurmurApiClient.h"

#include <juce_events/juce_events.h>

namespace pw8::plugin::net
{
    namespace
    {
        constexpr int kTimeoutMs = 8000;

        /// Reads the whole response body (JSON, always small here) and parses
        /// it -- returns a void var on any transport/parse failure so callers
        /// can fall back to a generic error message.
        juce::var readJsonBody(juce::InputStream* stream)
        {
            if (stream == nullptr)
                return {};
            const auto text = stream->readEntireStreamAsString();
            return juce::JSON::parse(text);
        }

        juce::String errorFromBody(const juce::var& body, int statusCode)
        {
            if (body.isObject())
            {
                const auto msg = body.getProperty("error", "").toString();
                if (msg.isNotEmpty())
                    return msg;
            }
            return "Request failed (HTTP " + juce::String(statusCode) + ").";
        }
    } // namespace

    juce::String MurmurApiClient::defaultBaseUrl()
    {
        if (const auto* override_ = std::getenv("MURMUR_WEB_API_BASE"); override_ != nullptr && *override_ != '\0')
            return juce::String(override_);

        // Real production deployment (docs/DEPLOY.md) -- plain HTTP, no
        // domain/TLS in front of it yet. Update this the day that changes.
        return "http://178.156.212.103:8080";
    }

    void MurmurApiClient::activate(const juce::String& licenseKey, std::function<void(ActivateResult)> callback)
    {
        const auto baseUrl = defaultBaseUrl();

        juce::Thread::launch([licenseKey, baseUrl, callback] {
            ActivateResult result;

            auto* bodyObj = new juce::DynamicObject();
            bodyObj->setProperty("licenseKey", licenseKey);
            const juce::String jsonBody = juce::JSON::toString(juce::var(bodyObj));

            juce::URL url(baseUrl + "/api/v1/activate");
            url = url.withPOSTData(jsonBody);

            int statusCode = 0;
            const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                                      .withExtraHeaders("Content-Type: application/json")
                                      .withConnectionTimeoutMs(kTimeoutMs)
                                      .withHttpRequestCmd("POST")
                                      .withStatusCode(&statusCode);

            auto stream = url.createInputStream(options);
            const auto body = readJsonBody(stream.get());

            if (stream == nullptr)
            {
                result.errorMessage = "Couldn't reach the MURMUR server. Check your connection.";
            }
            else if (statusCode < 200 || statusCode >= 300)
            {
                result.errorMessage = errorFromBody(body, statusCode);
            }
            else
            {
                result.success = true;
                const auto user = body.getProperty("user", {});
                result.displayName = user.getProperty("displayName", "").toString();
                result.email = user.getProperty("email", "").toString();
                result.entitled = static_cast<bool>(body.getProperty("entitled", false));
                result.keyName = body.getProperty("keyName", "").toString();
            }

            juce::MessageManager::callAsync([callback, result] { callback(result); });
        });
    }

    void MurmurApiClient::fetchLibrary(const juce::String& licenseKey, std::function<void(LibraryResult)> callback)
    {
        const auto baseUrl = defaultBaseUrl();

        juce::Thread::launch([licenseKey, baseUrl, callback] {
            LibraryResult result;

            juce::URL url(baseUrl + "/api/v1/library");

            int statusCode = 0;
            const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                      .withExtraHeaders("Authorization: Bearer " + licenseKey)
                                      .withConnectionTimeoutMs(kTimeoutMs)
                                      .withStatusCode(&statusCode);

            auto stream = url.createInputStream(options);
            const auto body = readJsonBody(stream.get());

            if (stream == nullptr)
            {
                result.errorMessage = "Couldn't reach the MURMUR server. Check your connection.";
            }
            else if (statusCode < 200 || statusCode >= 300)
            {
                result.errorMessage = errorFromBody(body, statusCode);
            }
            else
            {
                result.success = true;
                if (const auto* patchesArray = body.getProperty("patches", {}).getArray())
                {
                    for (const auto& p : *patchesArray)
                    {
                        LibraryPatch patch;
                        patch.slug = p.getProperty("slug", "").toString();
                        patch.name = p.getProperty("name", "").toString();
                        patch.category = p.getProperty("category", "").toString();
                        patch.downloadUrl = p.getProperty("downloadUrl", "").toString();
                        result.patches.add(std::move(patch));
                    }
                }
            }

            juce::MessageManager::callAsync([callback, result] { callback(result); });
        });
    }

} // namespace pw8::plugin::net
