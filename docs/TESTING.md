# Testing

## Current Status

**37 Catch2 test cases, 165,192 assertions, all passing** as of this pass
(`ctest --preset dev` / `./build/tests/pw8_tests`). Framework: Catch2 v3, fetched via
CMake `FetchContent` (see `tests/CMakeLists.txt`), registered with `ctest` via
`catch_discover_tests`.

| File | Covers |
|---|---|
| `tests/dsp/ClassicOscillatorTests.cpp` | Tuning accuracy (zero-crossing measurement) across waveforms, bounded/finite output under high-frequency PolyBLEP stress, continuous morph sweep stability, deterministic phase reset |
| `tests/dsp/DahdsrEnvelopeTests.cpp` | Sustain-level convergence, release-to-zero + idle transition, attack-duration timing accuracy, immediate reset |
| `tests/dsp/VoiceAllocatorTests.cpp` | Free-voice-first allocation, steal-released-before-gated policy, note+channel-scoped release |
| `tests/unit/AlgorithmGraphCompilerTests.cpp` | Default template compiles, feed-forward cycle rejection, FEEDBACK-typed loop acceptance, self-feedback, missing-output rejection, duplicate-ID rejection, out-of-range edge rejection, zero-edge elimination |
| `tests/unit/DeterministicRngTests.cpp` | Same-seed reproducibility, cross-seed divergence, `[0,1)` range, `deriveSeed` stability/decorrelation |
| `tests/serialization/PatchSerializerTests.cpp` | Full patch roundtrip (incl. algorithm graph), malformed-JSON rejection, non-object-root rejection, minimal-document defaulting |
| `tests/serialization/StandardMidiFileTests.cpp` | Hand-built minimal SMF parses correctly (tempo-to-seconds math verified), too-small-buffer and bad-magic-header rejection, `MidiSequence::durationSeconds()` |
| `tests/regression/RenderSanityTests.cpp` | Non-silent finite output for INIT SINE, correct silence with no MIDI input, 8-voice polyphonic overlap stays finite, out-of-range sample rate rejected, finite output under an aggressive self-feedback algorithm |

## What's NOT covered yet (see the empty/placeholder dirs for status)

- `tests/plugin/` -- pluginval/host-matrix, waits on the plugin actually building against JUCE.
- `tests/realtime/` -- automated allocation-interception verification that the audio
  path never allocates (currently enforced by code review + the fixed-capacity
  container discipline, not by a test).
- `tools/fuzz_render/` (`pw8-fuzz-render`) -- broad property/fuzz testing over
  randomly-generated valid patches. Not implemented; today's adversarial-input
  coverage is the small number of targeted cases above (self-feedback algorithm,
  malformed JSON/SMF, out-of-range options), not systematic fuzzing.
- `benchmarks/` -- Google Benchmark suite. Not implemented.
- Audio regression / golden-patch fingerprinting (peak/RMS/spectral-fingerprint
  tracking across engine changes). Not implemented -- `RenderSanityTests.cpp`
  asserts *sanity* (finite, non-silent, bounded) rather than *fingerprint stability*
  against a golden reference yet.

## Sanitizers

`PW8_ENABLE_ASAN` / `PW8_ENABLE_UBSAN` CMake options (`cmake/Sanitizers.cmake`),
applied to `pw8_core` and `pw8_tests` via `pw8_apply_sanitizers()`. Not run by
default; enable with e.g. `cmake --preset asan`. ThreadSanitizer is intentionally
not wired as a blanket flag (docs/BUILD.md) -- it's meant for targeted control-path
runs, not the realtime audio path, per the master spec.

## Static Analysis

`clang-format`/`clang-tidy` are not yet configured with a repo-specific config file
in this pass (PLANNED) -- `cmake/CompilerWarnings.cmake` applies a moderate
`-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused
-Woverloaded-virtual -Wnull-dereference` set (deliberately *not* `-Wconversion
-Wsign-conversion`, which would be extremely noisy for this codebase's float/size_t
mixing without much correctness payoff) via `pw8_set_warnings()`. `PW8_WARNINGS_AS_ERRORS`
is off by default.

## Running tests

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

See [BUILD.md](BUILD.md) for the full preset list.
