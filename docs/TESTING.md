# Testing

## Current Status

**118 Catch2 test cases, all passing** as of this pass (`ctest --preset dev` /
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
| `tests/unit/ModMatrixTests.cpp` | Neutral output with no routes, Velocity->FilterCutoff scaling, multiplicative OperatorLevel composition, inactive-route skipping, macro-index resolution, reading any of the 8 LFOs/envelopes by index, LFO sources reading the shared layer-wide tick at Layer/Global scope instead of the per-voice one, envelope sources ignoring declared scope |
| `tests/unit/FftTests.cpp` | `isPowerOfTwo` classification, forward+inverse roundtrip accuracy, single-bin sine peak isolation, safe no-op on non-power-of-two input |
| `tests/unit/WavetableTableTests.cpp` | Higher note frequency selects a more band-limited mip; **measured** aliasing-energy reduction (>2x) vs. always using the full-bandwidth mip |
| `tests/unit/ArpeggiatorTests.cpp` | Per-mode note-sequence generation (Up/Down/UpDown/AsPlayed hand-verified exact sequences), Chord mode firing every held note together, latch keeping the pattern alive after release vs. stopping without it, seeded-probability determinism, ratchet sub-hit count, tie suppressing retrigger, polymetric step/note-sequence independence |
| `tests/unit/EffectsTests.cpp` | Saturation transparency/compression; Chorus transparency and fixed-delay impulse response; TapeDelay Static echo spacing/decay and PingPong channel alternation; NodeDelay parent-child chaining and disabled-node exclusion; `FrequencyShifter`'s measured shift amount (FFT peak) and `FreqShiftEcho`'s bounded output; FractalEcho topology determinism/seed divergence/depth-scaling rule/finite output across a full morph sweep; `EffectChain` Bypass transparency and in-series processing; Reverb produces a decaying, spectrally-diffuse tail from an impulse and stays finite at maximum size/decay/feedback; Eq's low/mid/high bands each measurably shift RMS energy in the expected direction (via `dsp::Biquad` RBJ Cookbook formulas) and Bypass-equivalent settings stay transparent; Compressor reduces gain above threshold with soft-knee continuity and stays transparent below threshold; Limiter never lets a sustained loud signal exceed its ceiling and passes a quiet signal through effectively unchanged (empirical lag-search passthrough check) |
| `tests/unit/EngineMacroLiveUpdateTests.cpp` | `Engine::setMacroValue()` measurably changes a currently-held voice's output immediately (RMS drop/recovery via a macro->OperatorLevel mod route), not just the next note-on -- the property plugin macro automation depends on |
| `tests/unit/EngineLiveParamsTests.cpp` | The rest of Engine's "Live parameter API": a filter cutoff closing mid-hold measurably darkens a still-ringing voice, muting an operator's level mid-hold measurably silences it, an insert effect's saturation mid-hold measurably compresses a loud signal, and an arpeggiator rate change mid-pattern (`setArpeggiatorScalarLive`, not `configure()`) doesn't reset held notes/pattern position -- the property all 501 plugin-automatable parameters depend on |
| `tests/serialization/PatchSerializerTests.cpp` | Full patch roundtrip (incl. algorithm graph), malformed-JSON rejection, non-object-root rejection, minimal-document defaulting, v1->v2 migration of singular `ampEnvelope`/`lfo1` into `envelopes[0]`/`lfos[0]`, v1->v2 migration of `modRoutes[].source` ordinals to the reordered `ModSource` enum |
| `tests/serialization/StandardMidiFileTests.cpp` | Hand-built minimal SMF parses correctly (tempo-to-seconds math verified), too-small-buffer and bad-magic-header rejection, `MidiSequence::durationSeconds()` |
| `tests/serialization/WavetableTableLoaderTests.cpp` | Valid multi-mip table parses correctly, malformed-JSON/mismatched-sample-count/out-of-range-dimensions rejection, missing-file error reporting |
| `tests/regression/RenderSanityTests.cpp` | Non-silent finite output for INIT SINE, correct silence with no MIDI input, 8-voice polyphonic overlap stays finite, out-of-range sample rate rejected, finite output under an aggressive self-feedback algorithm, Filter 1 audibly changes spectrum (RMS drop), mod matrix (LFO/velocity/pressure -> filter/level/pan) renders finite audio, tempo-synced LFO's rate actually tracks `--bpm` end to end, enabling the arpeggiator turns one held chord into many discrete amplitude onsets (measured: 1 without, 16 with, at 8Hz/2s), a master TapeDelay slot turns one short hit into several measured echoes, a layer insert Saturation slot measurably lowers a loud patch's peak, a LAYER-scoped LFO route is one continuously-running shared clock (proven via windowed-RMS pan measurement) while a VOICE-scoped one resets per note-on, a master Limiter slot caps a sustained loud signal's peak strictly below its ceiling, a master Compressor slot (zero makeup gain) measurably lowers peak vs. the same patch without it |

## Property / Fuzz Testing

**IMPLEMENTED.** `pw8-fuzz-render` (`tools/fuzz_render/`) generates random-but-
schema-valid patches -- every value within its documented range, and the algorithm
graph constructed so it's *guaranteed* to compile (feed-forward edges only ever
route from a lower node index to a higher one, which is by construction acyclic;
`FEEDBACK`-typed edges, the only kind allowed to loop, are generated freely
including self-loops) -- renders each one, and asserts no crash, no NaN/Inf, bounded
output, and reasonable per-patch runtime. Filter 1, all 8 LFOs, all 8 envelopes,
and the mod matrix (all 29 sources including every LFO/envelope index, all 5
destinations, and all 3 scopes) are randomized too, deliberately including
extremes (maximum resonance, mod-route amounts up to +-48) to stress the
finite-output clamps added alongside them; BPM is randomized across [20, 300] to
exercise `TempoSync` LFOs. Verified across many batches, most recently: **5,000
patches (seed 1, pre-modulation), 5,000 more (seed 3, post-modulation), and 3,000
more (seed 4, post-wavetable-mip-mapping), 1,500 more (seed 5, post-arpeggiator
regression check -- `randomPatch()` does not yet randomize `ArpeggiatorParams`
itself, see below), 1,500 more (seed 6, post-FX-bank regression check --
`randomPatch()` does not yet randomize `EffectSlotParams` either, same gap),
1,800 more across seeds 9-12 (post-GATE-5: `randomPatch()` now randomizes all 8
LFOs/envelopes and the full 29-source/3-scope mod-route space, closing the
LFO/envelope half of the earlier gap), and 1,000 more (seed 13, post-GATE-10:
`randomPatch()` now fully randomizes every field of all 7 `EffectSlotParams`
slots -- 3 layer insert + 4 master, regardless of `type` -- across the complete
10-algorithm FX bank including the new Reverb/Eq/Compressor/Limiter, closing the
effects half of the earlier gap) -- zero failures in any of them** (Debug
build; a Release build would run substantially faster than the observed
~6-37 patches/sec, slower with the 8x LFO/envelope work and 7 fully-randomized
FX slots per patch). The master spec's overnight target of 1,000,000+ patches is
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

- `pw8-fuzz-render`'s `randomPatch()` does not yet randomize `ArpeggiatorParams`
  (mode/rate/steps/latch) the way it already randomizes filter/LFO/mod-route
  parameters -- the seed-5 batch above exercises the arpeggiator only in that it's
  present but disabled by default on random patches. PLANNED follow-up.
- `tests/plugin/` -- no `ctest`-registered plugin tests. A *manual* verification
  pass was done instead: `plugin/` builds against real JUCE 8.0.6, the AU target
  passes Apple's `auval` in full, and `pluginval --strictness-level 5` (the
  maximum) passes on both the VST3 and the AU (see `docs/PLUGIN_ARCHITECTURE.md`
  "pluginval") -- but `pluginval` was run locally (`brew install --cask
  pluginval`), not from CI; `.github/workflows/ci.yml`'s `plugin` job still only
  automates the build+`auval` check. A real DAW host-matrix pass is still PLANNED.
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
