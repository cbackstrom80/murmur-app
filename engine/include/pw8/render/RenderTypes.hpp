#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pw8::render
{
    enum class QualityMode : std::uint8_t
    {
        Eco = 0,
        Normal,
        High,
        Ultra,
        Offline,
    };

    [[nodiscard]] inline const char* toString(QualityMode q) noexcept
    {
        switch (q)
        {
            case QualityMode::Eco: return "eco";
            case QualityMode::Normal: return "normal";
            case QualityMode::High: return "high";
            case QualityMode::Ultra: return "ultra";
            case QualityMode::Offline: return "offline";
        }
        return "unknown";
    }

    struct RenderOptions
    {
        double sampleRate = 48000.0;
        int blockSize = 512;
        double bpm = 120.0;
        /// If >= 0, forces total render duration; otherwise duration is the MIDI
        /// sequence's last event time plus `releaseTailSeconds`.
        double durationSecondsOverride = -1.0;
        double releaseTailSeconds = 2.0;
        QualityMode quality = QualityMode::Offline;
        std::uint64_t seed = 0;
    };

    struct RenderMetrics
    {
        float peak = 0.0f;
        float rms = 0.0f;
        float dcOffsetLeft = 0.0f;
        float dcOffsetRight = 0.0f;
        bool containsNaNOrInf = false;
        double durationSeconds = 0.0;
        float leftRightBalance = 0.0f; ///< -1 (hard left) .. +1 (hard right), by RMS energy.
    };

    struct RenderResult
    {
        bool ok = false;
        std::string error;
        std::vector<float> interleavedStereo; ///< L, R, L, R, ...
        double sampleRate = 48000.0;
        RenderMetrics metrics{};
    };

} // namespace pw8::render
