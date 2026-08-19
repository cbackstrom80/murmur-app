#pragma once

#if JucePlugin_Build_Standalone

#include <atomic>
#include <functional>
#include <memory>

#include <juce_core/juce_core.h>

namespace pw8::plugin
{
    /// Localhost HTTP control plane for agentic patch building (MCP server → live Standalone).
    /// Binds 127.0.0.1 only; message-thread patch loads; manifest at
    /// ~/Library/Application Support/MURMUR/mcp-bridge.json.
    class StandaloneMcpBridge final : private juce::Thread
    {
    public:
        using LoadPatchFromPathFn = std::function<bool(const juce::String&)>;
        using LoadPatchFromJsonFn = std::function<bool(const juce::String&)>;
        using StatusSnapshotFn = std::function<juce::var()>;

        StandaloneMcpBridge(LoadPatchFromPathFn loadFromPath, LoadPatchFromJsonFn loadFromJson,
                            StatusSnapshotFn statusSnapshot);
        ~StandaloneMcpBridge() override;

        [[nodiscard]] int getPort() const noexcept { return port_.load(std::memory_order_acquire); }
        [[nodiscard]] bool isListening() const noexcept { return listening_.load(std::memory_order_acquire); }

    private:
        void run() override;
        void removeBridgeManifest() const;
        void writeBridgeManifest() const;
        void handleClient(std::unique_ptr<juce::StreamingSocket> client);
        [[nodiscard]] juce::String buildHttpResponse(int statusCode, const juce::String& statusText,
                                                     const juce::var& body) const;
        [[nodiscard]] bool dispatchLoadPath(const juce::String& path, juce::var& outBody) const;
        [[nodiscard]] bool dispatchLoadJson(const juce::String& jsonText, juce::var& outBody) const;

        LoadPatchFromPathFn loadFromPath_;
        LoadPatchFromJsonFn loadFromJson_;
        StatusSnapshotFn statusSnapshot_;

        std::unique_ptr<juce::StreamingSocket> listenSocket_;
        std::atomic<int> port_{0};
        std::atomic<bool> listening_{false};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StandaloneMcpBridge)
    };

} // namespace pw8::plugin

#endif
