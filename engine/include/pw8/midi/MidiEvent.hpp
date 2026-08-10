#pragma once

#include <cstdint>
#include <vector>

// Control-path MIDI representation used to drive the native renderer and (eventually)
// the plugin's MIDI input. This is NOT used inside the audio callback's per-sample
// loop -- events are consumed at block boundaries by pw8::render::Engine::process().

namespace pw8::midi
{
    enum class EventType : std::uint8_t
    {
        NoteOn,
        NoteOff,
        ControlChange,
        PitchBend,
        ChannelPressure,
        PolyAftertouch,
        ProgramChange,
    };

    struct MidiEvent
    {
        double timeSeconds = 0.0;
        EventType type = EventType::NoteOn;
        int channel = 0;      ///< 0-15
        int note = 0;         ///< 0-127 (NoteOn/NoteOff/PolyAftertouch)
        int velocity = 0;     ///< 0-127 (NoteOn/NoteOff)
        int controller = 0;   ///< 0-127 (ControlChange)
        int value = 0;        ///< 0-127 (ControlChange/ProgramChange), or 0-16383 (PitchBend, centered at 8192)
    };

    /// A time-ordered sequence of MIDI events, already resolved to absolute seconds
    /// (tempo already applied). See pw8::midi::readStandardMidiFile for how a .mid
    /// file with its own tempo map is converted into one of these.
    struct MidiSequence
    {
        std::vector<MidiEvent> events;

        [[nodiscard]] double durationSeconds() const noexcept
        {
            double maxT = 0.0;
            for (const auto& e : events)
                if (e.timeSeconds > maxT)
                    maxT = e.timeSeconds;
            return maxT;
        }
    };

} // namespace pw8::midi
