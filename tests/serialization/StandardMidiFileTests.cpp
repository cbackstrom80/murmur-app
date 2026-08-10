#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "pw8/midi/StandardMidiFile.hpp"

using namespace pw8::midi;

namespace
{
    void pushVlq(std::vector<std::uint8_t>& out, std::uint32_t value)
    {
        std::uint8_t buffer[5];
        int count = 0;
        buffer[count++] = static_cast<std::uint8_t>(value & 0x7F);
        value >>= 7;
        while (value > 0)
        {
            buffer[count++] = static_cast<std::uint8_t>((value & 0x7F) | 0x80);
            value >>= 7;
        }
        for (int i = count - 1; i >= 0; --i)
            out.push_back(buffer[i]);
    }

    void pushU32(std::vector<std::uint8_t>& out, std::uint32_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    void pushU16(std::vector<std::uint8_t>& out, std::uint16_t v)
    {
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
        out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    }

    /// Builds a minimal Format-0 SMF: a tempo meta event (120 BPM), a note-on for
    /// middle C, a note-off one quarter note later, and an end-of-track meta event.
    std::vector<std::uint8_t> buildTestSmf(std::uint16_t ticksPerQuarter = 480)
    {
        std::vector<std::uint8_t> track;
        pushVlq(track, 0);
        track.insert(track.end(), {0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20}); // 500000 us/quarter = 120 BPM.

        pushVlq(track, 0);
        track.insert(track.end(), {0x90, 0x3C, 0x64}); // Note on, ch0, note 60, vel 100.

        pushVlq(track, ticksPerQuarter); // one quarter note later.
        track.insert(track.end(), {0x80, 0x3C, 0x40}); // Note off, ch0, note 60, vel 64.

        pushVlq(track, 0);
        track.insert(track.end(), {0xFF, 0x2F, 0x00}); // End of track.

        std::vector<std::uint8_t> file;
        file.insert(file.end(), {'M', 'T', 'h', 'd'});
        pushU32(file, 6);
        pushU16(file, 0); // format 0
        pushU16(file, 1); // 1 track
        pushU16(file, ticksPerQuarter);

        file.insert(file.end(), {'M', 'T', 'r', 'k'});
        pushU32(file, static_cast<std::uint32_t>(track.size()));
        file.insert(file.end(), track.begin(), track.end());

        return file;
    }
} // namespace

TEST_CASE("readStandardMidiFile parses a minimal format-0 file", "[midi][smf]")
{
    const auto bytes = buildTestSmf();
    const auto result = readStandardMidiFile(bytes);

    REQUIRE(result.ok);
    REQUIRE(result.formatType == 0);
    REQUIRE(result.ticksPerQuarterNote == 480);
    REQUIRE(result.sequence.events.size() == 2);

    const auto& noteOn = result.sequence.events[0];
    REQUIRE(noteOn.type == EventType::NoteOn);
    REQUIRE(noteOn.note == 60);
    REQUIRE(noteOn.velocity == 100);
    REQUIRE(noteOn.timeSeconds == Catch::Approx(0.0));

    const auto& noteOff = result.sequence.events[1];
    REQUIRE(noteOff.type == EventType::NoteOff);
    REQUIRE(noteOff.note == 60);
    // One quarter note at 120 BPM == 0.5 seconds.
    REQUIRE(noteOff.timeSeconds == Catch::Approx(0.5).margin(0.001));
}

TEST_CASE("readStandardMidiFile rejects a too-small buffer", "[midi][smf][robustness]")
{
    const std::vector<std::uint8_t> tiny = {'M', 'T', 'h', 'd'};
    const auto result = readStandardMidiFile(tiny);
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("readStandardMidiFile rejects a bad magic header", "[midi][smf][robustness]")
{
    std::vector<std::uint8_t> bad = buildTestSmf();
    bad[0] = 'X';
    const auto result = readStandardMidiFile(bad);
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("MidiSequence::durationSeconds reflects the last event", "[midi]")
{
    const auto bytes = buildTestSmf();
    const auto result = readStandardMidiFile(bytes);
    REQUIRE(result.ok);
    REQUIRE(result.sequence.durationSeconds() == Catch::Approx(0.5).margin(0.001));
}
