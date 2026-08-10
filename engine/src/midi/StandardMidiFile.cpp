#include "pw8/midi/StandardMidiFile.hpp"

#include <algorithm>
#include <cstring>

namespace pw8::midi
{
    namespace
    {
        struct ByteReader
        {
            std::span<const std::uint8_t> data;
            std::size_t pos = 0;

            [[nodiscard]] bool hasMore(std::size_t n = 1) const noexcept { return pos + n <= data.size(); }

            [[nodiscard]] std::uint8_t u8() noexcept { return hasMore() ? data[pos++] : 0; }

            [[nodiscard]] std::uint32_t u32() noexcept
            {
                if (!hasMore(4)) { pos = data.size(); return 0; }
                std::uint32_t v = (static_cast<std::uint32_t>(data[pos]) << 24) |
                                   (static_cast<std::uint32_t>(data[pos + 1]) << 16) |
                                   (static_cast<std::uint32_t>(data[pos + 2]) << 8) |
                                   static_cast<std::uint32_t>(data[pos + 3]);
                pos += 4;
                return v;
            }

            [[nodiscard]] std::uint16_t u16() noexcept
            {
                if (!hasMore(2)) { pos = data.size(); return 0; }
                std::uint16_t v = static_cast<std::uint16_t>((data[pos] << 8) | data[pos + 1]);
                pos += 2;
                return v;
            }

            [[nodiscard]] std::uint32_t readVlq() noexcept
            {
                std::uint32_t value = 0;
                for (int i = 0; i < 5 && hasMore(); ++i)
                {
                    const std::uint8_t b = u8();
                    value = (value << 7) | (b & 0x7Fu);
                    if ((b & 0x80u) == 0)
                        break;
                }
                return value;
            }

            void skip(std::size_t n) noexcept { pos = std::min(data.size(), pos + n); }
        };

        struct RawEvent
        {
            std::uint64_t absoluteTick = 0;
            std::uint32_t trackIndex = 0;
            std::uint32_t orderInTrack = 0;
            bool isTempoMeta = false;
            std::uint32_t tempoUsPerQuarter = 500000;
            bool isChannelEvent = false;
            EventType type = EventType::NoteOn;
            int channel = 0;
            int a = 0; // note/controller/LSB
            int b = 0; // velocity/value/MSB
        };

        constexpr std::size_t kMaxTracks = 256;
        constexpr std::size_t kMaxEventsPerTrack = 2'000'000;

    } // namespace

    SmfLoadResult readStandardMidiFile(std::span<const std::uint8_t> bytes) noexcept
    {
        SmfLoadResult result;

        if (bytes.size() < 14)
        {
            result.error = "File too small to be a valid MIDI file";
            return result;
        }

        ByteReader r{bytes, 0};

        std::uint8_t headerMagic[4];
        for (auto& b : headerMagic) b = r.u8();
        if (std::memcmp(headerMagic, "MThd", 4) != 0)
        {
            result.error = "Missing MThd header chunk";
            return result;
        }

        const std::uint32_t headerLength = r.u32();
        if (headerLength < 6)
        {
            result.error = "Malformed MThd length";
            return result;
        }

        const std::uint16_t formatType = r.u16();
        const std::uint16_t numTracks = r.u16();
        const std::uint16_t division = r.u16();
        r.skip(headerLength - 6);

        if (division & 0x8000)
        {
            result.error = "SMPTE time division is not supported";
            return result;
        }
        if (numTracks == 0 || numTracks > kMaxTracks)
        {
            result.error = "Unsupported or invalid track count";
            return result;
        }

        result.formatType = formatType;
        result.ticksPerQuarterNote = division;

        std::vector<RawEvent> allEvents;

        for (std::uint16_t trackIdx = 0; trackIdx < numTracks; ++trackIdx)
        {
            if (!r.hasMore(8))
            {
                result.error = "Unexpected end of file while reading track header";
                return result;
            }

            std::uint8_t trackMagic[4];
            for (auto& b : trackMagic) b = r.u8();
            const std::uint32_t trackLength = r.u32();

            if (std::memcmp(trackMagic, "MTrk", 4) != 0 || !r.hasMore(trackLength))
            {
                result.error = "Malformed or truncated MTrk chunk";
                return result;
            }

            const std::size_t trackEnd = r.pos + trackLength;
            std::uint64_t absoluteTick = 0;
            std::uint8_t runningStatus = 0;
            std::uint32_t orderInTrack = 0;

            while (r.pos < trackEnd && orderInTrack < kMaxEventsPerTrack)
            {
                const std::uint32_t delta = r.readVlq();
                absoluteTick += delta;

                if (!r.hasMore())
                    break;

                std::uint8_t statusByte = r.u8();

                if (statusByte == 0xFF) // Meta event
                {
                    const std::uint8_t metaType = r.u8();
                    const std::uint32_t len = r.readVlq();

                    if (metaType == 0x51 && len == 3) // Set Tempo
                    {
                        const std::uint32_t us = (static_cast<std::uint32_t>(r.u8()) << 16) |
                                                  (static_cast<std::uint32_t>(r.u8()) << 8) |
                                                  static_cast<std::uint32_t>(r.u8());
                        RawEvent ev;
                        ev.absoluteTick = absoluteTick;
                        ev.trackIndex = trackIdx;
                        ev.orderInTrack = orderInTrack++;
                        ev.isTempoMeta = true;
                        ev.tempoUsPerQuarter = us > 0 ? us : 500000;
                        allEvents.push_back(ev);
                    }
                    else
                    {
                        r.skip(len);
                    }
                    continue;
                }

                if (statusByte == 0xF0 || statusByte == 0xF7) // SysEx
                {
                    const std::uint32_t len = r.readVlq();
                    r.skip(len);
                    continue;
                }

                // Running status: if the high bit isn't set, this byte is actually
                // data for the previous status byte.
                std::uint8_t data1;
                if (statusByte < 0x80)
                {
                    data1 = statusByte;
                    statusByte = runningStatus;
                }
                else
                {
                    runningStatus = statusByte;
                    data1 = r.u8();
                }

                const std::uint8_t hi = statusByte & 0xF0u;
                const int channel = statusByte & 0x0Fu;

                RawEvent ev;
                ev.absoluteTick = absoluteTick;
                ev.trackIndex = trackIdx;
                ev.orderInTrack = orderInTrack++;
                ev.isChannelEvent = true;
                ev.channel = channel;

                switch (hi)
                {
                    case 0x80: // Note off
                    {
                        const std::uint8_t vel = r.u8();
                        ev.type = EventType::NoteOff;
                        ev.a = data1;
                        ev.b = vel;
                        allEvents.push_back(ev);
                        break;
                    }
                    case 0x90: // Note on (velocity 0 == note off)
                    {
                        const std::uint8_t vel = r.u8();
                        ev.type = (vel == 0) ? EventType::NoteOff : EventType::NoteOn;
                        ev.a = data1;
                        ev.b = vel;
                        allEvents.push_back(ev);
                        break;
                    }
                    case 0xA0: // Poly aftertouch
                    {
                        const std::uint8_t val = r.u8();
                        ev.type = EventType::PolyAftertouch;
                        ev.a = data1;
                        ev.b = val;
                        allEvents.push_back(ev);
                        break;
                    }
                    case 0xB0: // Control change
                    {
                        const std::uint8_t val = r.u8();
                        ev.type = EventType::ControlChange;
                        ev.a = data1;
                        ev.b = val;
                        allEvents.push_back(ev);
                        break;
                    }
                    case 0xC0: // Program change (1 data byte)
                        ev.type = EventType::ProgramChange;
                        ev.a = data1;
                        allEvents.push_back(ev);
                        break;
                    case 0xD0: // Channel pressure (1 data byte)
                        ev.type = EventType::ChannelPressure;
                        ev.a = data1;
                        allEvents.push_back(ev);
                        break;
                    case 0xE0: // Pitch bend
                    {
                        const std::uint8_t msb = r.u8();
                        ev.type = EventType::PitchBend;
                        ev.a = data1;  // LSB
                        ev.b = msb;    // MSB
                        allEvents.push_back(ev);
                        break;
                    }
                    default:
                        // Unknown/unsupported status; nothing more to consume safely.
                        break;
                }
            }

            r.pos = trackEnd;
        }

        // Stable sort by (tick, track, order) so simultaneous events keep a deterministic order.
        std::stable_sort(allEvents.begin(), allEvents.end(), [](const RawEvent& a, const RawEvent& b) {
            if (a.absoluteTick != b.absoluteTick) return a.absoluteTick < b.absoluteTick;
            if (a.trackIndex != b.trackIndex) return a.trackIndex < b.trackIndex;
            return a.orderInTrack < b.orderInTrack;
        });

        const double ticksPerQuarter = static_cast<double>(division > 0 ? division : 480);
        double tempoUsPerQuarter = 500000.0;
        std::uint64_t lastTick = 0;
        double elapsedSeconds = 0.0;

        result.sequence.events.reserve(allEvents.size());

        for (const auto& ev : allEvents)
        {
            const double deltaTicks = static_cast<double>(ev.absoluteTick - lastTick);
            elapsedSeconds += (deltaTicks / ticksPerQuarter) * (tempoUsPerQuarter / 1'000'000.0);
            lastTick = ev.absoluteTick;

            if (ev.isTempoMeta)
            {
                tempoUsPerQuarter = static_cast<double>(ev.tempoUsPerQuarter);
                continue;
            }

            if (!ev.isChannelEvent)
                continue;

            MidiEvent out;
            out.timeSeconds = elapsedSeconds;
            out.type = ev.type;
            out.channel = ev.channel;

            switch (ev.type)
            {
                case EventType::NoteOn:
                case EventType::NoteOff:
                case EventType::PolyAftertouch:
                    out.note = ev.a;
                    out.velocity = ev.b;
                    break;
                case EventType::ControlChange:
                    out.controller = ev.a;
                    out.value = ev.b;
                    break;
                case EventType::PitchBend:
                    out.value = (ev.b << 7) | ev.a; // 14-bit, 8192 == center
                    break;
                case EventType::ChannelPressure:
                    out.value = ev.a;
                    break;
                case EventType::ProgramChange:
                    out.value = ev.a;
                    break;
            }

            result.sequence.events.push_back(out);
        }

        result.ok = true;
        return result;
    }

} // namespace pw8::midi
