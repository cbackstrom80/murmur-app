#!/usr/bin/env python3
"""Generates the Standard MIDI Files under content/test_midi/.

These are committed as binary .mid files, but this script is committed alongside
them so the fixtures are reproducible and auditable rather than opaque blobs.
Run from the repo root:

    python3 scripts/generate_test_midi.py
"""
import struct
import pathlib

OUT_DIR = pathlib.Path(__file__).resolve().parent.parent / "content" / "test_midi"


def vlq(value: int) -> bytes:
    """MIDI variable-length quantity encoding."""
    chunks = [value & 0x7F]
    value >>= 7
    while value > 0:
        chunks.append((value & 0x7F) | 0x80)
        value >>= 7
    return bytes(reversed(chunks))


def track(events: list[tuple[int, bytes]]) -> bytes:
    """events: list of (delta_ticks, raw_event_bytes), in order."""
    body = b"".join(vlq(delta) + data for delta, data in events)
    body += vlq(0) + bytes([0xFF, 0x2F, 0x00])  # End of track.
    return b"MTrk" + struct.pack(">I", len(body)) + body


def smf_format0(ticks_per_quarter: int, events: list[tuple[int, bytes]]) -> bytes:
    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, ticks_per_quarter)
    return header + track(events)


def tempo_event(bpm: float) -> bytes:
    us_per_quarter = round(60_000_000 / bpm)
    return bytes([0xFF, 0x51, 0x03]) + us_per_quarter.to_bytes(3, "big")


def note_on(channel: int, note: int, velocity: int) -> bytes:
    return bytes([0x90 | channel, note, velocity])


def note_off(channel: int, note: int, velocity: int = 64) -> bytes:
    return bytes([0x80 | channel, note, velocity])


def build_single_note() -> bytes:
    """One middle-C quarter note at 120 BPM. Simplest possible smoke test."""
    ppq = 480
    events = [
        (0, tempo_event(120)),
        (0, note_on(0, 60, 100)),
        (ppq, note_off(0, 60)),
    ]
    return smf_format0(ppq, events)


def build_bass_line() -> bytes:
    """A simple 8-note walking bass line at 105 BPM, for dark-bass.pw8 / sub-bass.pw8."""
    ppq = 480
    bpm = 105
    notes = [36, 36, 43, 41, 39, 41, 36, 34]  # low register.
    events: list[tuple[int, bytes]] = [(0, tempo_event(bpm))]
    for i, note in enumerate(notes):
        events.append((0 if i == 0 else 0, note_on(0, note, 105)))
        events.append((ppq, note_off(0, note)))
    return smf_format0(ppq, events)


def build_chord() -> bytes:
    """A held major triad (C-E-G) for two seconds, for soft-pad.pw8 / wide-saw.pw8."""
    ppq = 480
    bpm = 90
    notes = [48, 52, 55]
    events: list[tuple[int, bytes]] = [(0, tempo_event(bpm))]
    # All three notes start simultaneously (delta 0 between them).
    for i, note in enumerate(notes):
        events.append((0, note_on(0, note, 90)))
    hold_ticks = ppq * 4  # 4 quarter notes.
    for i, note in enumerate(notes):
        events.append((hold_ticks if i == 0 else 0, note_off(0, note)))
    return smf_format0(ppq, events)


def build_bell_phrase() -> bytes:
    """A few spaced single notes with room to decay, for fm-bell.pw8."""
    ppq = 480
    bpm = 96
    notes = [60, 64, 67, 72]
    events: list[tuple[int, bytes]] = [(0, tempo_event(bpm))]
    gap = ppq * 2  # 2 quarter notes between onsets.
    for i, note in enumerate(notes):
        events.append((gap if i > 0 else 0, note_on(0, note, 110)))
        events.append((ppq // 4, note_off(0, note)))
    return smf_format0(ppq, events)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    fixtures = {
        "single-note.mid": build_single_note(),
        "bass-line.mid": build_bass_line(),
        "chord.mid": build_chord(),
        "bell-phrase.mid": build_bell_phrase(),
    }
    for name, data in fixtures.items():
        path = OUT_DIR / name
        path.write_bytes(data)
        print(f"wrote {path} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
