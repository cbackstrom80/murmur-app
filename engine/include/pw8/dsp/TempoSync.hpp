#pragma once

#include <array>
#include <cstddef>

#include "pw8/dsp/Math.hpp"

// Shared musical tempo-sync helpers (LFO, arp, delay). Division lengths are
// expressed in quarter-note units — same convention as lfo::kSyncDivisionQuarterNotes.
namespace pw8::dsp
{
    inline constexpr std::array<float, 9> kDelaySyncDivisionQuarterNotes = {
        4.0f,        ///< 1/1 whole note
        2.0f,        ///< 1/2 half note
        1.0f,        ///< 1/4 quarter note (default)
        0.5f,        ///< 1/8
        0.25f,       ///< 1/16
        1.5f,        ///< dotted 1/4
        1.0f / 3.0f, ///< triplet 1/4
        0.75f,       ///< dotted 1/8
        1.0f / 6.0f, ///< triplet 1/8
    };

    inline constexpr std::size_t kDefaultDelaySyncDivisionIndex = 2; ///< 1/4 note.

    [[nodiscard]] inline float delayMsFromTempoSync(float bpm, int divisionIndex) noexcept
    {
        const auto idx = static_cast<std::size_t>(
            clamp(divisionIndex, 0, static_cast<int>(kDelaySyncDivisionQuarterNotes.size()) - 1));
        const float quarterNotes = kDelaySyncDivisionQuarterNotes[idx];
        const float bpmClamped = clamp(bpm, 20.0f, 999.0f);
        return 60000.0f / bpmClamped * quarterNotes;
    }

    [[nodiscard]] inline float effectiveDelayMs(bool syncEnabled, int divisionIndex, float freeMs, float bpm,
                                                float minMs, float maxMs) noexcept
    {
        const float ms = syncEnabled ? delayMsFromTempoSync(bpm, divisionIndex) : freeMs;
        return clamp(ms, minMs, maxMs);
    }

} // namespace pw8::dsp
