# Patch Format (`.pw8`)

**IMPLEMENTED** (schema v1, JSON, full load/save, untrusted-input hardening).

## Overview

`.pw8` is JSON internally (the spec allows this for developer ergonomics, with the
option to pack/compress later while keeping the logical schema). The data model
(`pw8::patch::Patch`, `pw8/patch/Patch.hpp`) has **no dependency on JSON at all** --
serialization is isolated entirely to `pw8/patch/PatchSerializer.hpp`/`.cpp`, so
`Patch.hpp` is safe to include anywhere without pulling in `nlohmann::json`.

Top-level shape:

```jsonc
{
  "schemaVersion": 1,
  "metadata": { "id", "name", "author", "description", "category", "moods", "genres",
                "tags", "createdAt", "engineVersion", "schemaVersion", "seed", "lineage" },
  "layerMode": 0,            // see LayerMode enum below
  "layerMorph": 0.0,
  "layerA": { /* LayerPatch, see below */ },
  "layerB": { /* LayerPatch */ },
  "voiceSettings": { "polyphony", "masterGain", "a4Hz" },
  "locks": { "lockSources", "lockAlgorithm", "lockFilters", "lockModulation", "lockEffects", "lockSequence" },
  "macros": [ /* 8x {"id","name","description","value"} */ ],
  "seed": 0
}
```

A `LayerPatch` contains: 8 `operators[]` (engine, waveform, morph, pulse width,
wavetable frame position, frequency ratio / fixed Hz / key-track, level, pan), an
`algorithm` graph definition (see ALGORITHM_GRAPH.md), an `ampEnvelope`, `unison`
settings (data model present, DSP wiring PLANNED -- see ROADMAP Phase 7), a
`filter1` (Filter 1 params: enabled/mode/cutoffHz/resonance/keyTrack -- IMPLEMENTED,
see DSP_ENGINE.md), an `lfo1` (waveform/mode/rateHz/syncDivisionIndex/phaseOffset --
IMPLEMENTED, see MODULATION.md), a `modRoutes` array (up to 64 `ModRoute`s --
IMPLEMENTED, see MODULATION.md), and `gain`/`pan`/`width`/`centerGravity`.

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

**PARTIAL** (mechanism exists, nothing to migrate yet -- there is only schema v1).
`PatchSerializer.cpp`'s internal `migrateToCurrentSchema(json&, int fromVersion)` is
the seam a v1->v2 step would be added to; today it's a no-op by construction. The
loader always stamps the *loaded* schema version into `PatchLoadResult::originalSchemaVersion`
before migration would run, so a future migration step can inspect what it's
migrating from. Real migration tests will be added once there's a second schema
version to migrate *to* -- adding one prematurely would just be testing a no-op.

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
