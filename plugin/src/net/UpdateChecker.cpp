#include "net/UpdateChecker.h"

#include <juce_events/juce_events.h>

namespace pw8::plugin::net
{
    namespace
    {
        constexpr int kTimeoutMs = 6000;
        constexpr const char* kLatestReleaseApiUrl =
            "https://api.github.com/repos/cbackstrom80/murmur-app/releases/latest";

        juce::StringArray parseVersionSegments(const juce::String& raw)
        {
            auto v = raw.trim();
            if (v.startsWithIgnoreCase("v"))
                v = v.substring(1);
            // Drop any pre-release/build suffix ("1.6.0-rc1" -> "1.6.0") -- a
            // simple dotted-numeric compare can't meaningfully order those.
            v = v.upToFirstOccurrenceOf("-", false, false);
            return juce::StringArray::fromTokens(v, ".", "");
        }
    } // namespace

    bool UpdateChecker::isNewerVersion(const juce::String& latest, const juce::String& current)
    {
        const auto a = parseVersionSegments(latest);
        const auto b = parseVersionSegments(current);
        const int n = juce::jmax(a.size(), b.size());

        for (int i = 0; i < n; ++i)
        {
            const int av = i < a.size() ? a[i].getIntValue() : 0;
            const int bv = i < b.size() ? b[i].getIntValue() : 0;
            if (av != bv)
                return av > bv;
        }
        return false; // equal
    }

    void UpdateChecker::checkForUpdate(std::function<void(UpdateCheckResult)> callback)
    {
        const juce::String currentVersion(JucePlugin_VersionString);

        juce::Thread::launch([currentVersion, callback] {
            UpdateCheckResult result;
            result.currentVersion = currentVersion;

            const juce::URL url(kLatestReleaseApiUrl);
            int statusCode = 0;
            const auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                      // GitHub's API rejects requests with no User-Agent.
                                      .withExtraHeaders("Accept: application/vnd.github+json\r\n"
                                                       "User-Agent: MURMUR-Plugin")
                                      .withConnectionTimeoutMs(kTimeoutMs)
                                      .withStatusCode(&statusCode);

            auto stream = url.createInputStream(options);
            if (stream != nullptr && statusCode >= 200 && statusCode < 300)
            {
                const auto body = juce::JSON::parse(stream->readEntireStreamAsString());
                const auto tagName = body.getProperty("tag_name", "").toString();
                const auto htmlUrl = body.getProperty("html_url", "").toString();

                if (tagName.isNotEmpty())
                {
                    result.success = true;
                    result.latestVersion = tagName.startsWithIgnoreCase("v") ? tagName.substring(1) : tagName;
                    result.releaseUrl = htmlUrl.isNotEmpty()
                                             ? htmlUrl
                                             : "https://github.com/cbackstrom80/murmur-app/releases/latest";
                    result.updateAvailable = isNewerVersion(result.latestVersion, currentVersion);
                }
            }

            juce::MessageManager::callAsync([callback, result] { callback(result); });
        });
    }

} // namespace pw8::plugin::net
