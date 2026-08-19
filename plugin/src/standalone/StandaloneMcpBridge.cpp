#if JucePlugin_Build_Standalone

#include "standalone/StandaloneMcpBridge.h"

#include <juce_events/juce_events.h>

#if JUCE_MAC || JUCE_LINUX || JUCE_BSD
#include <unistd.h>
#endif

namespace pw8::plugin
{
    namespace
    {
        constexpr int kMaxRequestBytes = 65536;
        constexpr int kLoadTimeoutMs = 10000;

        [[nodiscard]] juce::File bridgeManifestFile()
        {
            auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("MURMUR");
            dir.createDirectory();
            return dir.getChildFile("mcp-bridge.json");
        }

        [[nodiscard]] juce::String readRequestBody(juce::StreamingSocket& socket, int contentLength)
        {
            if (contentLength <= 0 || contentLength > kMaxRequestBytes)
                return {};

            juce::MemoryBlock block(static_cast<size_t>(contentLength));
            int totalRead = 0;
            auto* bytes = static_cast<char*>(block.getData());
            while (totalRead < contentLength)
            {
                const int n = socket.read(bytes + totalRead, contentLength - totalRead, false);
                if (n <= 0)
                    break;
                totalRead += n;
            }
            return juce::String::fromUTF8(static_cast<const char*>(block.getData()),
                                          static_cast<size_t>(totalRead));
        }

        [[nodiscard]] juce::String readHttpRequest(juce::StreamingSocket& socket)
        {
            juce::String request;
            request.preallocateBytes(4096);
            char byte = 0;
            while (request.length() < kMaxRequestBytes)
            {
                if (socket.read(&byte, 1, false) != 1)
                    break;
                request += byte;
                if (request.endsWith("\r\n\r\n"))
                    break;
            }
            return request;
        }

        [[nodiscard]] int parseContentLength(const juce::String& requestHeaders)
        {
            const auto lines = juce::StringArray::fromLines(requestHeaders);
            for (const auto& line : lines)
            {
                if (line.startsWithIgnoreCase("Content-Length:"))
                {
                    return line.fromFirstOccurrenceOf(":", false, false).trim().getIntValue();
                }
            }
            return 0;
        }

        [[nodiscard]] bool runOnMessageThread(const std::function<bool()>& fn)
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                return fn();

            juce::WaitableEvent done;
            std::atomic<bool> ok{false};
            juce::MessageManager::callAsync([&]() {
                ok.store(fn(), std::memory_order_relaxed);
                done.signal();
            });
            if (!done.wait(kLoadTimeoutMs))
                return false;
            return ok.load(std::memory_order_relaxed);
        }
    } // namespace

    StandaloneMcpBridge::StandaloneMcpBridge(LoadPatchFromPathFn loadFromPath,
                                             LoadPatchFromJsonFn loadFromJson,
                                             StatusSnapshotFn statusSnapshot)
        : juce::Thread("MURMUR MCP Bridge"),
          loadFromPath_(std::move(loadFromPath)),
          loadFromJson_(std::move(loadFromJson)),
          statusSnapshot_(std::move(statusSnapshot))
    {
        startThread(juce::Thread::Priority::background);
    }

    StandaloneMcpBridge::~StandaloneMcpBridge()
    {
        signalThreadShouldExit();
        if (listenSocket_ != nullptr)
            listenSocket_->close();
        waitForThreadToExit(5000);
        removeBridgeManifest();
    }

    void StandaloneMcpBridge::run()
    {
        listenSocket_ = std::make_unique<juce::StreamingSocket>();
        if (!listenSocket_->createListener(0, "127.0.0.1"))
        {
            juce::Logger::writeToLog("StandaloneMcpBridge: failed to bind localhost listener");
            return;
        }

        port_.store(listenSocket_->getPort(), std::memory_order_release);
        listening_.store(true, std::memory_order_release);
        writeBridgeManifest();

        juce::Logger::writeToLog("StandaloneMcpBridge: listening on 127.0.0.1:"
                                 + juce::String(port_.load()) + " (MCP agents can load patches live)");

        while (!threadShouldExit())
        {
            if (!listenSocket_->waitUntilReady(true, 250))
                continue;

            auto client = std::unique_ptr<juce::StreamingSocket>(listenSocket_->waitForNextConnection());
            if (client == nullptr || threadShouldExit())
                continue;

            handleClient(std::move(client));
        }

        listening_.store(false, std::memory_order_release);
    }

    void StandaloneMcpBridge::writeBridgeManifest() const
    {
        juce::DynamicObject::Ptr root(new juce::DynamicObject());
        root->setProperty("version", 1);
        root->setProperty("host", "127.0.0.1");
        root->setProperty("port", port_.load(std::memory_order_acquire));
#if JUCE_MAC || JUCE_LINUX || JUCE_BSD
        root->setProperty("pid", static_cast<juce::int64>(getpid()));
#else
        root->setProperty("pid", static_cast<juce::int64>(0));
#endif
        root->setProperty("product", "MURMUR");
        bridgeManifestFile().replaceWithText(juce::JSON::toString(juce::var(root.get()), true));
    }

    void StandaloneMcpBridge::removeBridgeManifest() const
    {
        const auto manifest = bridgeManifestFile();
        if (manifest.existsAsFile())
            manifest.deleteFile();
    }

    juce::String StandaloneMcpBridge::buildHttpResponse(int statusCode, const juce::String& statusText,
                                                        const juce::var& body) const
    {
        const auto json = juce::JSON::toString(body, true);
        juce::String response;
        response << "HTTP/1.1 " << statusCode << ' ' << statusText << "\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Connection: close\r\n";
        response << "Content-Length: " << json.getNumBytesAsUTF8() << "\r\n\r\n";
        response << json;
        return response;
    }

    bool StandaloneMcpBridge::dispatchLoadPath(const juce::String& path, juce::var& outBody) const
    {
        if (path.isEmpty() || loadFromPath_ == nullptr)
        {
            juce::DynamicObject::Ptr body(new juce::DynamicObject());
            body->setProperty("ok", false);
            body->setProperty("error", "invalid_request");
            outBody = juce::var(body.get());
            return false;
        }

        const bool ok = runOnMessageThread([&]() { return loadFromPath_(path); });
        juce::DynamicObject::Ptr body(new juce::DynamicObject());
        body->setProperty("ok", ok);
        body->setProperty("path", path);
        if (!ok)
            body->setProperty("error", "load_failed");
        outBody = juce::var(body.get());
        return ok;
    }

    bool StandaloneMcpBridge::dispatchLoadJson(const juce::String& jsonText, juce::var& outBody) const
    {
        if (jsonText.isEmpty() || loadFromJson_ == nullptr)
        {
            juce::DynamicObject::Ptr body(new juce::DynamicObject());
            body->setProperty("ok", false);
            body->setProperty("error", "invalid_request");
            outBody = juce::var(body.get());
            return false;
        }

        const bool ok = runOnMessageThread([&]() { return loadFromJson_(jsonText); });
        juce::DynamicObject::Ptr body(new juce::DynamicObject());
        body->setProperty("ok", ok);
        if (!ok)
            body->setProperty("error", "load_failed");
        outBody = juce::var(body.get());
        return ok;
    }

    void StandaloneMcpBridge::handleClient(std::unique_ptr<juce::StreamingSocket> client)
    {
        if (client == nullptr)
            return;

        const auto request = readHttpRequest(*client);
        if (request.isEmpty())
            return;

        const auto headerEnd = request.indexOf("\r\n\r\n");
        const auto headerSection = headerEnd >= 0 ? request.substring(0, headerEnd) : request;
        const auto requestLine = headerSection.upToFirstOccurrenceOf("\r\n", false, false);
        const auto method = requestLine.upToFirstOccurrenceOf(" ", false, false).trim();
        const auto target = requestLine.fromFirstOccurrenceOf(" ", false, false)
                                .upToFirstOccurrenceOf(" ", false, false)
                                .trim();

        const int contentLength = parseContentLength(headerSection);
        auto bodyText = readRequestBody(*client, contentLength);

        juce::var responseBody;
        int statusCode = 404;
        juce::String statusText = "Not Found";

        if (method.equalsIgnoreCase("GET") && target == "/v1/status")
        {
            statusCode = 200;
            statusText = "OK";
            if (statusSnapshot_ != nullptr)
                responseBody = statusSnapshot_();
            else
            {
                juce::DynamicObject::Ptr body(new juce::DynamicObject());
                body->setProperty("ok", true);
                body->setProperty("product", "MURMUR");
                responseBody = juce::var(body.get());
            }
        }
        else if (method.equalsIgnoreCase("POST") && target == "/v1/load_path")
        {
            const auto parsed = juce::JSON::parse(bodyText);
            const auto path = parsed.getProperty("path", {}).toString();
            statusCode = dispatchLoadPath(path, responseBody) ? 200 : 400;
            statusText = statusCode == 200 ? "OK" : "Bad Request";
        }
        else if (method.equalsIgnoreCase("POST") && target == "/v1/load_json")
        {
            const auto parsed = juce::JSON::parse(bodyText);
            const juce::String jsonPayload =
                parsed.hasProperty("patch") ? juce::JSON::toString(parsed.getProperty("patch", {}), false)
                                            : bodyText;
            statusCode = dispatchLoadJson(jsonPayload, responseBody) ? 200 : 400;
            statusText = statusCode == 200 ? "OK" : "Bad Request";
        }
        else if (method.equalsIgnoreCase("OPTIONS"))
        {
            statusCode = 204;
            statusText = "No Content";
            juce::String response = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
            client->write(response.toRawUTF8(), static_cast<int>(response.getNumBytesAsUTF8()));
            return;
        }

        const auto response = buildHttpResponse(statusCode, statusText, responseBody);
        client->write(response.toRawUTF8(), static_cast<int>(response.getNumBytesAsUTF8()));
    }

} // namespace pw8::plugin

#endif
