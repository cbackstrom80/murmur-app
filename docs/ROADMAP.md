# Roadmap

Status as of this repository's initial engineering pass. Legend: **DONE** / **PARTIAL** / **PLANNED**.

| Phase | Scope | Status |
|---|---|---|
| 0 | Repository foundation (structure, CMake, core library skeleton, tests, benchmarks, docs, CI, coding standards) | **DONE** |
| 1 | First sound (Engine, Voice, VoiceAllocator, ClassicOscillator, DAHDSR, MIDI note handling, stereo output, native renderer) | **DONE** |
| 2 | Wavetable (preprocessing, mipmapping, frame interpolation, wavetable source) | **PARTIAL** -- oscillator + builder tool implemented; mip-mapping/band-limiting not yet done (see DSP_ENGINE.md) |
| 3 | 8-node algorithm graph (nodes, edges, validation, compiler, compiled execution, audio routing) | **DONE**, and beyond AUDIO-only -- all 7 edge types implemented |
| 4 | PM/FM/AM/RING typed modulation edges | **DONE** at the graph level (stretch goal achieved); a dedicated Engine Type 3 (FM/PM) with its own ratio/fixed-frequency modes is still PLANNED |
| 5 | Modulation (envelopes, LFO, mod matrix, macros, performance controls) | **PARTIAL** -- amplitude envelope, one per-voice LFO (6 waveforms, 4 modes incl. tempo-sync), and a VOICE-scoped mod matrix (6 sources incl. 8 macros, 4 destinations) are all IMPLEMENTED; 8-envelope/8-LFO/LAYER+GLOBAL-scope targets PLANNED |
| 6 | Filters (clean multimode, first character filter) | **PARTIAL** -- Filter 1 (TPT state-variable: LP/HP/BP/notch/peak, per-voice, mod-matrix-modulatable cutoff/resonance, key tracking) is **IMPLEMENTED**; Filter 2 (nonlinear character filter) PLANNED |
| 7 | Unison / stereo (unison, voice drift, pan, width, center gravity) | **PARTIAL** -- data model present (`UnisonSettings`, `centerGravity` on `LayerPatch`); DSP wiring PLANNED. `content/presets/wide-saw.pw8` demonstrates the *effect* today via hand-detuned operators rather than an automated unison engine |
| 8 | Dual layer (Layer A, Layer B, stack, layer morph) | **PARTIAL** -- schema complete (`LayerMode` enum, full `layerB` data), only `SINGLE_A` is actually voiced/rendered |
| 9 | Algorithm morph (same-topology, different-topology) | **PLANNED** |
| 10 | Additional engines (additive, phase/shape, noise, resonator, granular) | **PLANNED** -- `EngineType` enum and dispatch points exist; each renders silence until implemented |
| 11 | FX (insert slots, master slots, first effect set, reverb, spatial engine) | **PLANNED** |
| 12 | MSEG / sequencer | **PLANNED** |
| 13 | Patch format productionization (schema, migrations, metadata) | **PARTIAL** -- schema v1 complete and hardened against untrusted input; migration *mechanism* exists with nothing to migrate yet (only one schema version so far) |
| 14 | Python API productionization | **PARTIAL** -- see PYTHON_API.md coverage table |
| 15 | Patchwork integration (Sound IR compilation boundary) | **PARTIAL** -- CLI/Python/schema boundaries exist and work; `EightPatchCompiler` (IR -> Patch) itself is PLANNED, see PATCHWORK_INTEGRATION.md |
| 16 | Plugin (VST3, AU, Standalone) | **PARTIAL, build-verified** -- builds against real JUCE 8.0.6; AU passes `auval` in full; Standalone launches cleanly; no UI/automation/host-matrix yet. See PLUGIN_ARCHITECTURE.md |
| 17 | UI | **PLANNED** (deliberately, per spec) -- `createEditor()` returns JUCE's generic placeholder editor, not a step toward the real UI |
| 18 | AI features (Generate, Mutate, Breed, Lock) via Patchwork | **PLANNED** -- metadata hooks (`LockFlags`, `lineage`, deterministic seeding) exist; the AI pipeline itself lives in Patchwork |
| 19 | Factory bank (512-1024 curated presets) | **PLANNED** -- 7 engineering test patches exist (`content/presets/`), not factory-curated content |
| 20 | Production hardening (host matrix, pluginval, auval, VST validator, fuzz tests, soak tests, perf optimization) | **PARTIAL** -- `auval` passes in full (see Phase 16); `pw8-fuzz-render` implemented and run (5,000 patches, 0 failures); `pluginval`, host matrix, and soak testing still PLANNED |

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

## Competitive check: Serum 2 & Phase Plant

See [COMPETITIVE_ANALYSIS.md](COMPETITIVE_ANALYSIS.md) for the full feature-parity
matrix. Summary: every gap identified (modulator count, oscillator variety, effects
breadth, UI) was already tracked under an existing PLANNED phase above -- this
research validated the existing phase scope rather than changing it. The algorithm
graph, deterministic rendering, and native headless rendering are genuine
differentiators neither competitor has. Per the master spec's explicit prohibition
on copying UI layout/visual identity from commercial products, no UI design or code
resulted from this pass -- only a feature/gap comparison.

## Follow-up pass: what "build it all" added

A second pass built and verified everything that was previously scaffolded or
merely described:

- `benchmarks/` -- real Google Benchmark suite (oscillators, algorithm graph, voice,
  full-patch render at the master spec's exact 44.1/48/96 kHz x 1/8/16/32-voice
  matrix), built and run.
- `tools/fuzz_render/` (`pw8-fuzz-render`) -- implemented and run: 5,000
  randomly-generated, schema-valid, compiler-guaranteed-acyclic patches, zero
  failures, zero NaN/Inf.
- `plugin/` -- actually built against JUCE (bumped 7.0.12 -> 8.0.6 after 7.0.12
  failed to compile against the current macOS SDK). VST3/AU/Standalone all build;
  **AU passes Apple's `auval` validation tool in full**; a real bug was caught and
  fixed in the process (`hasEditor()`/`createEditor()` inconsistency -- JUCE's own
  assertion in `AudioProcessor::createEditorIfNeeded()` caught it when the
  Standalone app was actually launched, not by inspection).
- `bindings/python` -- rebuilt from a clean preset and re-smoke-tested.
- CI gained a macOS `plugin` job (build + `auval`, non-blocking) and a Linux+macOS
  `python-bindings` job (build + smoke test).

## Follow-up pass: Filter 1, LFO, and the mod matrix

A third pass implemented the two items this doc had previously called out as the
highest-leverage next steps (items 2 and 3 below, prior to this pass):

- **Filter 1** (`pw8/filter/StateVariableFilter.hpp`): TPT state-variable filter,
  LP/HP/BP/notch/peak, per-voice, wired into the signal chain
  (Algorithm Graph -> Filter 1 -> Amp Envelope -> Pan -> Output), with key tracking.
- **LFO1** (`pw8/lfo/Lfo.hpp`): 6 waveforms, 4 modes (free/retrigger/one-shot/
  tempo-sync), per-voice, deterministically seeded, tempo threaded end-to-end from
  `RenderOptions::bpm` / the JUCE host playhead through `Engine::setTempo()`.
- **Mod matrix** (`pw8/modulation/`): VOICE-scoped, 6 source types (LFO1, amp
  envelope, velocity, channel pressure, poly aftertouch, MPE slide) plus all 8
  macros, 4 destination types (filter cutoff/resonance, per-operator level, pan),
  fixed-capacity (64 routes), no realtime allocation.
- 19 new tests (filter stability/frequency-response, LFO rate/mode/determinism, mod
  matrix routing/composition, plus 4 full-Engine regression tests proving the
  filter/LFO/tempo/mod-matrix chain actually changes rendered audio end to end) --
  56 total, all passing.
- `pw8-fuzz-render` extended to randomize filter/LFO/mod-route parameters
  (deliberately including max resonance and extreme mod amounts); 5,000-patch batch,
  zero failures.
- Three factory presets updated to use real modulation instead of only
  oscillator-choice tricks: `dark-bass.pw8` (envelope+velocity -> filter cutoff,
  replacing its previous "darkness via morph only" approach), `soft-pad.pw8` (slow
  LFO -> filter cutoff for genuine movement), `wide-saw.pw8` (velocity -> filter
  cutoff brightness).
- A competitive-research pass (see above) confirmed the macro model already matches
  Phase Plant's "8 routable macros" structurally, and validated that no roadmap
  rescoping was needed.

## Immediate next steps (suggested, not committed)

1. Wavetable mip-mapping (finishes Phase 2 properly).
2. `juce::AudioProcessorValueTreeState` parameter wiring + `pluginval` + a real DAW
   host-matrix pass -- the plugin now builds and passes `auval`, so this is the
   natural next increment toward a genuinely shippable plugin rather than a
   from-scratch effort.
3. Expand modulation to the full 8-envelope/8-LFO/LAYER+GLOBAL-scope target now that
   the 1-of-each VOICE-scoped version has proven the data model and execution
   pattern end to end.
4. Filter 2 (nonlinear character filter) -- Filter 1's TPT SVF proved the per-voice
   filter integration point; a second filter stage is now a smaller increment than
   it was before Filter 1 existed.
