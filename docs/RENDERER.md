# Native Offline Renderer

**IMPLEMENTED.** `pw8::render::render()` (`pw8/render/Renderer.hpp`,
`engine/src/render/Renderer.cpp`) renders a `Patch` against a `MidiSequence`
entirely natively -- no JUCE plugin hosting, no DAW, no DawDreamer. This is a core
feature, not a test harness bolted on afterward: it's what `murmur-render`, the Python
bindings' `render()`/`Engine.render()`, and (eventually) headless factory-content
generation all go through.

## API

```cpp
RenderResult render(const Patch& patch, const MidiSequence& midi, const RenderOptions& options) noexcept;

// Reuses an already-prepared Engine (e.g. one that already had loadPatch() called),
// rather than constructing a throwaway one -- what Engine.render() uses in Python.
RenderResult renderWithEngine(Engine& engine, const MidiSequence& midi, const RenderOptions& options) noexcept;
```

`RenderOptions`: `sampleRate`, `blockSize` (internal chunking only, not a DSP
requirement), `bpm` (informational -- MIDI file tempo events take precedence),
`durationSecondsOverride` (defaults to MIDI length + `releaseTailSeconds`),
`releaseTailSeconds` (default 2.0), `quality` (plumbed through, not yet DSP-consumed
-- see DSP_ENGINE.md), `seed`.

`RenderResult`: `ok`, `error`, `interleavedStereo` (L, R, L, R, ... `float`),
`sampleRate`, `metrics`.

## Error Model

Explicit status results, not exceptions, in the DSP/render path (exceptions are used
in the JSON/tooling layer, e.g. `PatchSerializer`'s internal try/catch around
`nlohmann::json` calls -- see `docs/ARCHITECTURE.md` "Error Model"). `render()`
itself is `noexcept`; out-of-range `sampleRate`/`blockSize` return
`RenderResult{ok=false, error="..."}` rather than crashing or asserting. Total
render duration is clamped to a 1-hour sanity ceiling regardless of what a MIDI file
or override requests.

## MIDI Input

`midi::readStandardMidiFile()` (`pw8/midi/StandardMidiFile.hpp`,
`engine/src/midi/StandardMidiFile.cpp`) is a hand-rolled, dependency-free Standard
MIDI File reader: Format 0/1, multi-track (merged and tempo-resolved into absolute
seconds), running status, tempo map (0x51 meta events, mid-file tempo changes
honored), note on/off (velocity-0 note-on treated as note-off), control change,
pitch bend, channel pressure, poly aftertouch, program change (parsed but not yet
acted on -- see PATCHWORK_INTEGRATION.md). SysEx and other meta events are skipped,
not errors. Treated as untrusted input: truncated/malformed files return
`SmfLoadResult{ok=false}` rather than reading out of bounds --
`tests/serialization/StandardMidiFileTests.cpp` covers both the happy path (with a
hand-built minimal SMF byte buffer, verified tempo-to-seconds conversion) and the
too-small/bad-magic-header rejection cases.

`content/test_midi/*.mid` are real binary SMF files, generated (and reproducible)
via `scripts/generate_test_midi.py`: `single-note.mid`, `bass-line.mid` (8-note
walking bass), `chord.mid` (held triad), `bell-phrase.mid` (spaced single notes with
decay room).

## WAV Output

`render::writeWavFileFloat32()` (`pw8/render/WavWriter.hpp`) writes 32-bit IEEE
float PCM WAV (format tag 3) -- no dithering/clipping decisions needed since it
matches the renderer's internal float pipeline exactly. 16/24-bit integer PCM
delivery formats are a PLANNED addition that wouldn't touch the render pipeline
itself.

## DSP Metrics / Render Receipt

`RenderMetrics`: `peak`, `rms`, `dcOffsetLeft`/`dcOffsetRight`, `containsNaNOrInf`,
`durationSeconds`, `leftRightBalance` (RMS-energy-based, -1..+1). Computed by a
single pass over the rendered buffer in `Renderer.cpp`'s `computeMetrics()`.

`murmur-render --receipt <path.json>` writes a render receipt with real values (engine
version, patch schema version, patch/MIDI paths, sample rate, bpm, seed, duration,
and the full metrics block) -- see `tools/render/main.cpp`. This is intentionally
*not* the full Patchwork "PieceJudge" QA system, just the native-render-level QA the
master spec calls out.

## CLI

```
murmur-render --patch content/presets/dark-bass.murmur \
            --midi content/test_midi/bass-line.mid \
            --bpm 105 --sample-rate 48000 \
            --output /tmp/dark-bass.wav [--receipt /tmp/dark-bass.receipt.json] \
            [--duration <seconds>] [--release-tail <seconds>] [--seed <n>]
```

Smoke-tested against all 7 factory presets and all 4 test MIDI fixtures during
development (see the repo's development log / commit history) -- clean finite
output, sensible peak/RMS, `containsNaNOrInf: false` in every case, including the
deliberately-adversarial self-feedback-algorithm case.
