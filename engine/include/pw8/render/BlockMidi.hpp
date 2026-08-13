#pragma once

#include <cstddef>
#include <cstdint>

// Sample-accurate MIDI events consumed inside Engine::process(). Events must be
// sorted ascending by sampleOffset and scoped to the current block.

namespace pw8::render
{
    enum class BlockMidiType : std::uint8_t
    {
        NoteOn = 0,
        NoteOff,
        PitchBend,
        ControlChange,
        ChannelPressure,
        PolyAftertouch,
    };

    struct BlockMidiEvent
    {
        std::size_t sampleOffset = 0;
        BlockMidiType type = BlockMidiType::NoteOn;
        int channel = 0;
        int note = 0;
        int velocity = 0;   ///< 0-127 for note on/off; poly aftertouch value
        int controller = 0; ///< CC number
        int value = 0;      ///< CC value or 14-bit pitch bend (0-16383)
    };

} // namespace pw8::render
