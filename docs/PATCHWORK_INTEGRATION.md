# Patchwork Integration Boundary

Patchwork Eight is a **standalone repository**, deliberately not coupled to the
existing Patchwork AI repository. Patchwork consumes this repository as a
dependency/service/library through one of four boundaries:

1. **CLI** -- `pw8-render`, `pw8-info`, `pw8-graph`, `pw8-wavetable-builder`
   (`tools/`). Stable, file-in/file-out, no shared process state. This is the
   lowest-integration-risk boundary and works today for any headless render
   pipeline (see [RENDERER.md](RENDERER.md)).
2. **Python API** -- `patchwork_eight` (`bindings/python/`). See
   [PYTHON_API.md](PYTHON_API.md) for current coverage. This is the natural
   integration point for Patchwork's generation/scoring loop (candidate rendering,
   QA metrics) once it needs in-process rather than subprocess calls.
3. **Patch schema** -- `.pw8` JSON, versioned (`schemaVersion`), documented in
   [PATCH_FORMAT.md](PATCH_FORMAT.md). Patchwork can read/write this directly
   without linking any Patchwork Eight code at all, as long as it respects the
   schema.
4. **Sound IR adapter boundary** -- see below. **PLANNED**, not implemented.

## Sound IR

Patchwork's internal "Sound IR" (its engine-agnostic sound description) and
Patchwork Eight's native `Patch` are deliberately **not** the same representation --
forcing them to be identical would compromise the native patch format's own design
(see PATCH_FORMAT.md's operator/algorithm-graph specificity, which has no reason to
exist in a general-purpose cross-engine IR).

The intended compilation boundary:

```
Patchwork Sound IR
        |
        v
  EightPatchCompiler   (PLANNED -- not implemented in this pass)
        |
        v
   pw8::patch::Patch
```

`EightPatchCompiler` doesn't exist yet. What this pass establishes instead is the
*target side* of that compiler: a complete, documented, versioned `Patch` schema
(PATCH_FORMAT.md) and a `PatchLoadResult`/`savePatchToJson` API stable enough to
compile into. Building the actual IR->Patch compiler is Patchwork-repository work
(or a later phase here) once the Sound IR's shape is settled on the Patchwork side.

## Generate / Mutate / Breed / Lock

Per the master spec, **AI inference does not run inside this repository's audio
thread, and doesn't run inside this repository at all in this pass.** Generate/
Mutate/Breed/directed-mutation belong in Patchwork (background process / Python),
which talks to Patchwork Eight only through structured patches and the render API.

What Patchwork Eight *does* provide today to make that pipeline possible later:

- `LockFlags` on every patch (`lockSources`/`lockAlgorithm`/`lockFilters`/
  `lockModulation`/`lockEffects`/`lockSequence`) -- generation-control metadata the
  engine carries faithfully but doesn't interpret itself.
- `PatchMetadata::lineage` -- parent patch IDs, for breeding provenance.
- Deterministic, seeded rendering end-to-end (`dsp::DeterministicRng`,
  `Patch::seed`, `RenderOptions::seed`) -- required for reproducible AI evaluation
  of generated candidates (identical patch + seed + MIDI must render identically).
- A render pipeline fast enough and scriptable enough (native CLI + Python API) to
  drive the "64-256 candidates -> render -> QA -> score -> diversity-cluster -> top
  candidates" Generate workflow described in the master spec, without needing a DAW
  or plugin host in the loop.

Typed patch component boundaries (`LayerPatch`, `AlgorithmGraphDefinition`,
`OperatorPatch`, etc. as distinct structs rather than one flat parameter blob) exist
specifically so a future semantic-crossover Breed implementation (e.g. "child =
parent A's algorithm + parent B's modulation") has clean seams to cut along, rather
than needing to invent that structure retroactively.

## Program Change

`midi::EventType::ProgramChange` is parsed by the SMF reader and dispatched as a
no-op by `render::Engine`/`Renderer` (`dispatchEvent()`'s `ProgramChange` case is
explicitly empty with a comment). Patch-per-program-change is a Patchwork
integration concern (e.g. mapping MIDI program numbers to specific `.pw8` files) --
out of scope for `pw8_core` itself.
