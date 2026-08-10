# Patch Format (`.pw8`)

**IMPLEMENTED** (schema v2, JSON, full load/save, untrusted-input hardening,
v1->v2 migration).

## Overview

`.pw8` is JSON internally (the spec allows this for developer ergonomics, with the
option to pack/compress later while keeping the logical schema). The data model
(`pw8::patch::Patch`, `pw8/patch/Patch.hpp`) has **no dependency on JSON at all** --
serialization is isolated entirely to `pw8/patch/PatchSerializer.hpp`/`.cpp`, so
`Patch.hpp` is safe to include anywhere without pulling in `nlohmann::json`.

Top-level shape:

```jsonc
{
  "schemaVersion": 2,
  "metadata": { "id", "name", "author", "description", "category", "moods", "genres",
                "tags", "createdAt", "engineVersion", "schemaVersion", "seed", "lineage" },
  "layerMode": 0,            // see LayerMode enum below
  "layerMorph": 0.0,
  "layerA": { /* LayerPatch, see below */ },
  "layerB": { /* LayerPatch */ },
  "voiceSettings": { "polyphony", "masterGain", "a4Hz" },
  "locks": { "lockSources", "lockAlgorithm", "lockFilters", "lockModulation", "lockEffects", "lockSequence" },
  "macros": [ /* 8x {"id","name","description","value"} */ ],
  "arpeggiator": { /* ArpeggiatorParams -- performance-wide, see ARPEGGIATOR.md */ },
  "masterEffects": [ /* 4x EffectSlotParams -- final mixed bus, see FX_BANK.md */ ],
  "seed": 0
}
```

`arpeggiator` (`sequencer::ArpeggiatorParams` -- **IMPLEMENTED**, see
[ARPEGGIATOR.md](ARPEGGIATOR.md)) is performance-wide rather than per-layer,
since MIDI note dispatch itself is engine-wide: `enabled`, `mode` (Up/Down/UpDown/
DownUp/AsPlayed/Random/Chord), `rateMode` (Free/TempoSync), `rateHz`,
`syncDivisionIndex`, `octaveRange`, `numSteps`, `latch`, and up to 64 `steps[]`
(`ArpStep`: enabled, octaveOffset, gate, probability, ratchetCount, tie,
velocityScale, accent). `numSteps` is re-derived from the parsed `steps[]` array
length on load if omitted, clamped to `[1, 64]`.

A `LayerPatch` contains: 8 `operators[]` (engine, waveform, morph, pulse width,
wavetable frame position, frequency ratio / fixed Hz / key-track, level, pan), an
`algorithm` graph definition (see ALGORITHM_GRAPH.md), an `envelopes` array (8x
`DahdsrParams` -- IMPLEMENTED, see MODULATION.md; index 0 is conventionally "the"
amp envelope, all 8 are equally usable mod matrix sources), `unison` settings (data
model present, DSP wiring PLANNED -- see ROADMAP Phase 7), a `filter1` (Filter 1
params: enabled/mode/cutoffHz/resonance/keyTrack -- IMPLEMENTED, see DSP_ENGINE.md),
a `lfos` array (8x `LfoParams` -- waveform/mode/rateHz/syncDivisionIndex/phaseOffset,
IMPLEMENTED, VOICE + LAYER/GLOBAL scope, see MODULATION.md), a `modRoutes` array
(up to 64 `ModRoute`s, each with a 29-value `source` enum -- IMPLEMENTED, see
MODULATION.md), `gain`/`pan`/`width`/`centerGravity`, and an `insertEffects` array
(3x `EffectSlotParams` -- IMPLEMENTED, see FX_BANK.md).

`EffectSlotParams` (see FX_BANK.md for the full field-by-field rationale) is a
flat struct with a `type` (`EffectType`: Bypass/Saturation/Chorus/TapeDelay/
NodeDelay/FreqShiftEcho/FractalEcho/Reverb/Eq/Compressor/Limiter -- 10 real
algorithms plus Bypass), a shared `mix`, and fields for every type -- only the
active `type`'s fields matter, the rest are simply unused, the same pattern
`OperatorPatch` uses for its 5 engine types. `nodes[]` (used by NodeDelay) is
6x `DelayNodeParams` (enabled, parentIndex, delayMs, feedback, pan, distortion,
level). The struct carries 43 scalar fields in total (GATE 10 added 20 for the
four newest algorithms): `reverbSizeParam`/`reverbDecaySeconds`/
`reverbDampingHz`/`reverbPreDelayMs` (Reverb, a 4-line Householder-matrix FDN);
`eqLowFreqHz`/`eqLowGainDb`/`eqMidFreqHz`/`eqMidGainDb`/`eqMidQ`/`eqHighFreqHz`/
`eqHighGainDb` (Eq, 3-band via RBJ Cookbook biquads); `compThresholdDb`/
`compRatio`/`compAttackMs`/`compReleaseMs`/`compKneeDb`/`compMakeupDb`
(Compressor, feedforward soft-knee, stereo-linked); `limiterCeilingDb`/
`limiterLookaheadMs`/`limiterReleaseMs` (Limiter, true lookahead,
sliding-window-minimum gain).

See `content/presets/*.pw8` for complete, real, loadable examples, and
`tests/serialization/PatchSerializerTests.cpp` for the roundtrip contract.

## `LayerMode`

`SINGLE_A`, `SINGLE_B`, `STACK`, `SPLIT`, `VELOCITY_SPLIT`, `MORPH`, `KEY_ZONE` all
exist in the schema/enum today so patches authored now keep their meaning as later
phases land. **Only `SINGLE_A` is exercised by the render path in this pass** --
`render::Engine` always compiles and voices `layerA`; `layerB` is fully present in
the data model (round-trips, validates) but is not mixed into audio output yet. See
ROADMAP.md Phase 8 (dual layer) / Phase 9 (algorithm/layer morph).

## Migration

**IMPLEMENTED** -- a real v1->v2 step now exists, exercising the mechanism for the
first time (see `PatchSerializer.cpp`'s `migrateToCurrentSchema(json&, int fromVersion)`).
Schema v2 (docs/MODULATION.md "8 envelopes / 8 LFOs", GATE 5 pass) changed two
things a v1 document needs migrating for:

1. **`LayerPatch`'s singular `ampEnvelope`/`lfo1` fields became 8-slot
   `envelopes[]`/`lfos[]` arrays.** A v1 document's single object becomes index 0
   of the new array; the rest default. Applied independently to `layerA`/`layerB`.
2. **`ModRoute.source`'s enum ordinals shifted** when `Lfo2`-`Lfo8`/`Env1`-`Env8`
   were inserted between `Lfo1` and `Velocity` (old 15-value enum:
   `None=0,Lfo1=1,AmpEnvelope=2,Velocity=3,...`; new 29-value enum:
   `None=0,Lfo1..Lfo8=1-8,Env1..Env8=9-16,Velocity=17,...`). Every `modRoutes[].source`
   value in a v1 document is remapped through an explicit old-ordinal -> new-ordinal
   table. This was a real bug caught before release, not a hypothetical: without
   it, a v1 preset's `"source": 2` (AmpEnvelope) would have silently loaded as the
   new enum's `Lfo2` -- the wrong source, not a load failure, so it would never have
   surfaced as an error. Two of this repo's own shipped presets
   (`dark-bass.pw8`, `wide-saw.pw8`) were in exactly this shape.

The loader always stamps the *loaded* schema version into
`PatchLoadResult::originalSchemaVersion` before migration runs, so a future
migration step can inspect what it's migrating from; the `if (fromVersion < N)`
structure in `migrateToCurrentSchema()` is written so a future v2->v3 step can be
appended, not inserted into the middle of v1->v2's logic.
`tests/serialization/PatchSerializerTests.cpp` covers both migration steps against
hand-written v1 JSON documents (not just synthetic round-trips through the current
schema), including the exact `"source": 2`/`"source": 3` shape the real presets had.

## Security / Robustness

`.pw8` files are treated as untrusted input (docs/ARCHITECTURE.md's "audio thread
never touches unvalidated data" principle extends to "control-path code never trusts
a file just because it parsed"):

- Hard 8 MB ceiling on input size before parsing is even attempted.
- Every fixed-capacity container (`core::FixedVector`, `std::array<..., 8>`) clamps
  writes at capacity instead of growing -- a hostile `edges` array with 10,000
  entries can't cause unbounded allocation, extras are silently dropped.
- Every numeric field is range-clamped on the way in (see `clampNum()` calls
  throughout `PatchSerializer.cpp`).
- String-list fields (`moods`/`genres`/`tags`/`lineage`) are capped at 64 entries
  each after parsing.
- Parse errors and structurally-invalid documents return
  `PatchLoadResult{ok=false, error="..."}`, never a partially-constructed patch
  presented as valid. See `tests/serialization/PatchSerializerTests.cpp`'s
  malformed-JSON and non-object-root cases.
- A malformed/adversarial algorithm graph inside a patch cannot reach the audio
  thread regardless -- it goes through the same `AlgorithmGraphCompiler::compile()`
  validation as any other graph, with the same known-safe fallback on failure.

## Wavetable Resource Resolution

**PARTIAL.** An operator on the Wavetable engine (`OperatorPatch::engine == 1`)
names its table via `wavetableId` (a string). In this pass, `wavetableId` is
resolved as a plain **filesystem path** -- relative to the process's current
working directory, or absolute -- and loaded directly by
`render::Engine::loadPatch()` via `oscillator::loadWavetableFromFile()`. There is
no content-addressed ID registry, search-path configuration, or de-duplication of
tables shared across operators yet (each operator slot that references a table
loads and owns its own copy in memory, even if two slots name the same file). A
missing or malformed file does not fail the whole patch load -- that operator just
renders silence, matching `WavetableOscillator`'s existing "invalid view -> silence"
contract. `content/presets/wt-morph.pw8` names
`"content/wavetables/basic_harmonic.json"` and works correctly when run from the
repo root (e.g. via `pw8-render` or the Python bindings, both of which resolve
relative paths against the caller's working directory). A proper content-addressed
resolution system, matching the rest of the content pipeline, is PLANNED.

## Metadata & Categories

Category vocabulary (bass, lead, pluck, keyboard, pad, arp, drone, chord, fx, brass,
vocal texture, strings, sequence) and character/mood tags (warm, dark, bright,
aggressive, dreamy, metallic, glassy, organic, digital, lush, gritty, clean,
distorted, ambient, punchy, soft, evolving, airy) are plain strings, not hardwired
DSP enums, per the master spec -- `PatchMetadata::category`/`moods`/`tags` are
`std::string`/`std::vector<std::string>`.

## AI-Generation Metadata

`LockFlags` (`lockSources`/`lockAlgorithm`/`lockFilters`/`lockModulation`/
`lockEffects`/`lockSequence`) and `PatchMetadata::lineage` (parent patch IDs) are
present in the schema now as generation-control metadata for the future
Generate/Mutate/Breed pipeline (which lives in Patchwork, not here -- see
PATCHWORK_INTEGRATION.md). They are not interpreted by `pw8_core` itself; the engine
just carries them faithfully through load/save.
