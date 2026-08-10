# Testing

## Current Status

**67 Catch2 test cases, all passing** as of this pass (`ctest --preset dev` /
`./build/tests/pw8_tests`). Framework: Catch2 v3.8.1, fetched via CMake
`FetchContent` (see `tests/CMakeLists.txt`), registered with `ctest` via
`catch_discover_tests`. (Catch2 was bumped from v3.6.0 to v3.8.1 after v3.6.0's
`catch_discover_tests` mis-parsed the test list under this project's CMake version,
silently collapsing 16 of 37 test cases into one bogus combined-name `ctest` entry --
individually-discovered `ctest` output was confirmed correct after the bump.)

| File | Covers |
|---|---|
| `tests/dsp/ClassicOscillatorTests.cpp` | Tuning accuracy (zero-crossing measurement) across waveforms, bounded/finite output under high-frequency PolyBLEP stress, continuous morph sweep stability, deterministic phase reset |
| `tests/dsp/DahdsrEnvelopeTests.cpp` | Sustain-level convergence, release-to-zero + idle transition, attack-duration timing accuracy, immediate reset |
| `tests/dsp/VoiceAllocatorTests.cpp` | Free-voice-first allocation, steal-released-before-gated policy, note+channel-scoped release |
| `tests/dsp/StateVariableFilterTests.cpp` | Lowpass/highpass/bandpass frequency-response direction, stability and bounded output at maximum resonance across the full cutoff range, clean state reset |
| `tests/dsp/LfoTests.cpp` | Sine range/rate accuracy, square-wave exact +-1 output, Retrigger phase-reset determinism, OneShot hold-after-one-cycle, TempoSync BPM-to-rate math, SampleHold determinism-for-seed |
| `tests/unit/AlgorithmGraphCompilerTests.cpp` | Default template compiles, feed-forward cycle rejection, FEEDBACK-typed loop acceptance, self-feedback, missing-output rejection, duplicate-ID rejection, out-of-range edge rejection, zero-edge elimination |
| `tests/unit/DeterministicRngTests.cpp` | Same-seed reproducibility, cross-seed divergence, `[0,1)` range, `deriveSeed` stability/decorrelation |
| `tests/unit/ModMatrixTests.cpp` | Neutral output with no routes, Velocity->FilterCutoff scaling, multiplicative OperatorLevel composition, inactive-route skipping, macro-index resolution |
| `tests/unit/FftTests.cpp` | `isPowerOfTwo` classification, forward+inverse roundtrip accuracy, single-bin sine peak isolation, safe no-op on non-power-of-two input |
| `tests/unit/WavetableTableTests.cpp` | Higher note frequency selects a more band-limited mip; **measured** aliasing-energy reduction (>2x) vs. always using the full-bandwidth mip |
| `tests/serialization/PatchSerializerTests.cpp` | Full patch roundtrip (incl. algorithm graph), malformed-JSON rejection, non-object-root rejection, minimal-document defaulting |
| `tests/serialization/StandardMidiFileTests.cpp` | Hand-built minimal SMF parses correctly (tempo-to-seconds math verified), too-small-buffer and bad-magic-header rejection, `MidiSequence::durationSeconds()` |
| `tests/serialization/WavetableTableLoaderTests.cpp` | Valid multi-mip table parses correctly, malformed-JSON/mismatched-sample-count/out-of-range-dimensions rejection, missing-file error reporting |
| `tests/regression/RenderSanityTests.cpp` | Non-silent finite output for INIT SINE, correct silence with no MIDI input, 8-voice polyphonic overlap stays finite, out-of-range sample rate rejected, finite output under an aggressive self-feedback algorithm, Filter 1 audibly changes spectrum (RMS drop), mod matrix (LFO/velocity/pressure -> filter/level/pan) renders finite audio, tempo-synced LFO's rate actually tracks `--bpm` end to end |

## Property / Fuzz Testing

**IMPLEMENTED.** `pw8-fuzz-render` (`tools/fuzz_render/`) generates random-but-
schema-valid patches -- every value within its documented range, and the algorithm
graph constructed so it's *guaranteed* to compile (feed-forward edges only ever
route from a lower node index to a higher one, which is by construction acyclic;
`FEEDBACK`-typed edges, the only kind allowed to loop, are generated freely
including self-loops) -- renders each one, and asserts no crash, no NaN/Inf, bounded
output, and reasonable per-patch runtime. Filter 1, LFO1, and the mod matrix are
randomized too, deliberately including extremes (maximum resonance, mod-route
amounts up to +-48, all 5 mod destinations including `OperatorWavetablePosition`) to
stress the finite-output clamps added alongside them; BPM is randomized across
[20, 300] to exercise `TempoSync` LFOs. Verified across three batches: **5,000
patches (seed 1, pre-modulation), 5,000 more (seed 3, post-modulation), and 3,000
more (seed 4, post-wavetable-mip-mapping) -- zero failures in any of them** (Debug
build; a Release build would run substantially faster than the observed
~28-37 patches/sec). The master spec's overnight target of 1,000,000+ patches is
left to whoever runs it -- `--count`/`--seed` are both exposed for exactly that.

```bash
./build/dev/tools/pw8-fuzz-render --count 10000 --seed 1
```

## Benchmarks

**IMPLEMENTED.** `benchmarks/` (Google Benchmark, `PW8_BUILD_BENCHMARKS`/`benchmarks`
preset) covers the Classic and Wavetable oscillators (at 44.1/48/96 kHz), three
algorithm-graph topologies (parallel-8, serial-8, feedback-bell), a single-voice
render loop, and full-patch rendering across the master spec's exact matrix
(44.1/48/96 kHz x 1/8/16/32 voices). Sample captured results (Debug build on this
machine -- not representative of Release performance, see the preset's build-type
warning): full-patch render of a 1-second clip ranged from ~1.8ms (44.1kHz, 1 voice)
to ~5.0ms (96kHz, 32 voices), comfortably realtime even unoptimized.

## What's NOT covered yet

- `tests/plugin/` -- no `ctest`-registered plugin tests. A *manual* verification
  pass was done instead: `plugin/` builds against real JUCE 8.0.6 and the AU target
  passes Apple's `auval` in full (see `docs/PLUGIN_ARCHITECTURE.md`); `.github/workflows/ci.yml`'s
  `plugin` job automates the build+`auval` check on macOS. `pluginval` and a real
  DAW host-matrix are still PLANNED.
- `tests/realtime/` -- automated allocation-interception verification that the audio
  path never allocates (currently enforced by code review + the fixed-capacity
  container discipline, not by a test).
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
