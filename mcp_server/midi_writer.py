"""A minimal, dependency-free Standard MIDI File writer -- just enough to
build a one-chord preview note list for render_preview()/validate_patch() in
server.py. Deliberately hand-rolled rather than adding a `mido` dependency:
this MCP server should run with nothing beyond `pip install mcp` plus an
already-built murmur-render, matching this project's general preference for
minimal, obvious dependencies (see e.g. Knob3D's hand-rolled matrix math to
avoid an uncertain juce::Matrix3D dependency)."""
from __future__ import annotations

import struct


def _vlq(value: int) -> bytes:
    """MIDI variable-length quantity encoding."""
    buf = [value & 0x7F]
    value >>= 7
    while value:
        buf.insert(0, (value & 0x7F) | 0x80)
        value >>= 7
    return bytes(buf)


def write_chord_midi(path, notes: list[int], velocity: int = 90, hold_seconds: float = 1.5,
                      bpm: int = 120, ticks_per_beat: int = 480) -> None:
    """Writes a format-0 SMF: all `notes` on together at t=0, all off together
    at t=hold_seconds. Good enough for a preview render -- not a real
    composition tool."""
    ticks_per_sec = ticks_per_beat * bpm / 60.0
    hold_ticks = max(1, round(hold_seconds * ticks_per_sec))

    events = bytearray()
    events += _vlq(0)
    tempo_usec = round(60_000_000 / bpm)
    events += bytes([0xFF, 0x51, 0x03]) + tempo_usec.to_bytes(3, "big")

    for i, note in enumerate(notes):
        events += _vlq(0)
        events += bytes([0x90, note & 0x7F, velocity & 0x7F])

    for i, note in enumerate(notes):
        events += _vlq(hold_ticks if i == 0 else 0)
        events += bytes([0x80, note & 0x7F, 0])

    events += _vlq(0)
    events += bytes([0xFF, 0x2F, 0x00])  # end of track

    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, ticks_per_beat))
        f.write(b"MTrk" + struct.pack(">I", len(events)) + bytes(events))
