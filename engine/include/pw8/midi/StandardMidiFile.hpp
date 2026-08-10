#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "pw8/midi/MidiEvent.hpp"

// Minimal Standard MIDI File (SMF) reader: Format 0 and Format 1, multi-track,
// running status, tempo map (0x51 meta events), note on/off, control change,
// pitch bend, channel pressure, poly aftertouch, program change. SysEx and other
// meta events are skipped (not an error). Treats the file as untrusted input:
// truncated/malformed data returns a failure result rather than reading out of bounds.
//
// This is intentionally hand-rolled rather than a dependency -- SMF parsing is small,
// stable, and having no external dependency for something this fundamental to the
// renderer's input path is worth the ~200 lines.

namespace pw8::midi
{
    struct SmfLoadResult
    {
        bool ok = false;
        MidiSequence sequence{};
        std::string error;
        int formatType = 0;
        int ticksPerQuarterNote = 480;
    };

    [[nodiscard]] SmfLoadResult readStandardMidiFile(std::span<const std::uint8_t> bytes) noexcept;

} // namespace pw8::midi
