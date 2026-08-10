# Roadmap

Status as of this repository's initial engineering pass. Legend: **DONE** / **PARTIAL** / **PLANNED**.

| Phase | Scope | Status |
|---|---|---|
| 0 | Repository foundation (structure, CMake, core library skeleton, tests, benchmarks scaffold, docs, CI, coding standards) | **DONE** |
| 1 | First sound (Engine, Voice, VoiceAllocator, ClassicOscillator, DAHDSR, MIDI note handling, stereo output, native renderer) | **DONE** |
| 2 | Wavetable (preprocessing, mipmapping, frame interpolation, wavetable source) | **PARTIAL** -- oscillator + builder tool implemented; mip-mapping/band-limiting not yet done (see DSP_ENGINE.md) |
| 3 | 8-node algorithm graph (nodes, edges, validation, compiler, compiled execution, audio routing) | **DONE**, and beyond AUDIO-only -- all 7 edge types implemented |
| 4 | PM/FM/AM/RING typed modulation edges | **DONE** at the graph level (stretch goal achieved); a dedicated Engine Type 3 (FM/PM) with its own ratio/fixed-frequency modes is still PLANNED |
| 5 | Modulation (envelopes, LFO, mod matrix, macros, performance controls) | **PARTIAL** -- amplitude envelope + MPE-shaped per-note expression capture done; LFO, mod matrix, macro routing PLANNED |
| 6 | Filters (clean multimode, first character filter) | **PLANNED** |
| 7 | Unison / stereo (unison, voice drift, pan, width, center gravity) | **PARTIAL** -- data model present (`UnisonSettings`, `centerGravity` on `LayerPatch`); DSP wiring PLANNED. `content/presets/wide-saw.pw8` demonstrates the *effect* today via hand-detuned operators rather than an automated unison engine |
| 8 | Dual layer (Layer A, Layer B, stack, layer morph) | **PARTIAL** -- schema complete (`LayerMode` enum, full `layerB` data), only `SINGLE_A` is actually voiced/rendered |
| 9 | Algorithm morph (same-topology, different-topology) | **PLANNED** |
| 10 | Additional engines (additive, phase/shape, noise, resonator, granular) | **PLANNED** -- `EngineType` enum and dispatch points exist; each renders silence until implemented |
| 11 | FX (insert slots, master slots, first effect set, reverb, spatial engine) | **PLANNED** |
| 12 | MSEG / sequencer | **PLANNED** |
| 13 | Patch format productionization (schema, migrations, metadata) | **PARTIAL** -- schema v1 complete and hardened against untrusted input; migration *mechanism* exists with nothing to migrate yet (only one schema version so far) |
| 14 | Python API productionization | **PARTIAL** -- see PYTHON_API.md coverage table |
| 15 | Patchwork integration (Sound IR compilation boundary) | **PARTIAL** -- CLI/Python/schema boundaries exist and work; `EightPatchCompiler` (IR -> Patch) itself is PLANNED, see PATCHWORK_INTEGRATION.md |
| 16 | Plugin (VST3, AU, Standalone) | **PARTIAL (untested scaffold)** -- see PLUGIN_ARCHITECTURE.md |
| 17 | UI | **PLANNED** (deliberately, per spec) |
| 18 | AI features (Generate, Mutate, Breed, Lock) via Patchwork | **PLANNED** -- metadata hooks (`LockFlags`, `lineage`, deterministic seeding) exist; the AI pipeline itself lives in Patchwork |
| 19 | Factory bank (512-1024 curated presets) | **PLANNED** -- 7 engineering test patches exist (`content/presets/`), not factory-curated content |
| 20 | Production hardening (host matrix, pluginval, auval, VST validator, fuzz tests, soak tests, perf optimization) | **PLANNED** |

## This pass's actual deliverable

Per the master spec's "First Build Target" section (Phase 0 + Phase 1 solidly, plus
enough of Phase 2/3 to prove the architecture, with wavetable + PM as stretch
goals), this pass shipped:

- A building, testable repository (`cmake --preset dev && cmake --build --preset dev && ctest --preset dev`)
- MIDI-in, polyphonic (configurable, default 16 / max 32 voices), stable-amplitude,
  click-managed-at-note-boundaries native audio rendering
- Sine/saw/square/triangle via band-limited (PolyBLEP) oscillators, tuning-verified
- Envelope (DAHDSR)
- Stable `.pw8` patch serialization with untrusted-input hardening
- 8 operator slots per layer, always
- A data-driven algorithm graph, compiled and executed with all 7 edge types
  (exceeding the Phase 3 "AUDIO-only if needed" bar)
- WAV rendering (32-bit float) with a real render-receipt QA JSON
- Python callability (pybind11, built and smoke-tested)
- Both stretch goals: a working (if not yet mip-mapped) wavetable source, and
  working phase-modulation edges (exceeding "basic PM edge" -- full FM/PM/AM/RM/
  SYNC/FEEDBACK typed edges)

## Immediate next steps (suggested, not committed)

1. Wavetable mip-mapping (finishes Phase 2 properly).
2. Mod matrix + LFO (Phase 5) -- unlocks meaningful use of the macro/expression data
   already captured.
3. Filter 1 (clean multimode) -- the single highest-leverage missing piece for
   subjective sound quality (Phase 6).
4. `pw8-fuzz-render` (`tools/fuzz_render/`) -- the algorithm graph's
   fuzz-safety properties (bounded feedback, finite-output clamps, compiler
   validation) are architecturally in place; a real fuzz harness would give
   confidence at scale rather than the current handful of targeted adversarial tests.
