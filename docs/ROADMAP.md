# Roadmap

Status as of this repository's initial engineering pass. Legend: **DONE** / **PARTIAL** / **PLANNED**.

| Phase | Scope | Status |
|---|---|---|
| 0 | Repository foundation (structure, CMake, core library skeleton, tests, benchmarks, docs, CI, coding standards) | **DONE** |
| 1 | First sound (Engine, Voice, VoiceAllocator, ClassicOscillator, DAHDSR, MIDI note handling, stereo output, native renderer) | **DONE** |
| 2 | Wavetable (preprocessing, mipmapping, frame interpolation, wavetable source) | **DONE** -- oscillator, builder tool, and FFT-based mip-mapping/band-limiting all implemented and proven to reduce aliasing by a measured >2x (see DSP_ENGINE.md); a real factory table and preset (`wt-morph.pw8`) exercise the full pipeline end to end |
| 3 | 8-node algorithm graph (nodes, edges, validation, compiler, compiled execution, audio routing) | **DONE**, and beyond AUDIO-only -- all 7 edge types implemented |
| 4 | PM/FM/AM/RING typed modulation edges | **DONE** at the graph level (stretch goal achieved); a dedicated Engine Type 3 (FM/PM), a self-contained 2-operator FM voice in one node, is now also **IMPLEMENTED** (see DSP_ENGINE.md) |
| 5 | Modulation (envelopes, LFO, mod matrix, macros, performance controls) | **IMPLEMENTED** -- 8 envelopes and 8 LFOs per layer (VOICE + LAYER/GLOBAL scope for LFOs), a mod matrix with 29 sources and 5 destinations, and 8 macros, all live-automatable via the plugin. See "GATE 5" below |
| 6 | Filters (clean multimode, first character filter) | **PARTIAL** -- Filter 1 (TPT state-variable: LP/HP/BP/notch/peak, per-voice, mod-matrix-modulatable cutoff/resonance, key tracking) is **IMPLEMENTED**; Filter 2 (nonlinear character filter) PLANNED |
| 7 | Unison / stereo (unison, voice drift, pan, width, center gravity) | **PARTIAL** -- data model present (`UnisonSettings`, `centerGravity` on `LayerPatch`); DSP wiring PLANNED. `content/presets/wide-saw.murmur` demonstrates the *effect* today via hand-detuned operators rather than an automated unison engine |
| 8 | Dual layer (Layer A, Layer B, stack, layer morph) | **PARTIAL** -- schema complete (`LayerMode` enum, full `layerB` data), only `SINGLE_A` is actually voiced/rendered |
| 9 | Algorithm morph (same-topology, different-topology) | **PLANNED** |
| 10 | Additional engines (additive, phase/shape, noise, resonator, granular) | **DONE** -- NoiseChaos (Engine Type 7), PhaseShape (Engine Type 5), Additive (Engine Type 4), Resonator (Engine Type 8), and Granular (Engine Type 6) all **IMPLEMENTED**: 7 noise variants seeded via `dsp::DeterministicRng`, CZ-style phase distortion + wavefold, a 64-partial coupled-form oscillator bank with tilt/odd-even/stretch, a noise-excited modal filter bank via a new `Biquad::setBandpass()` primitive, and a fixed-capacity grain pool reading the same wavetableId-loaded data the Wavetable engine uses, respectively -- see DSP_ENGINE.md. Each landed as a separate, independently-verified PR. This closes out Phase 10's full engine set; all 8 `EngineType`s now render real audio. (FM/PM, also once part of this phase, ships under Phase 4 above.) |
| 11 | FX (insert slots, master slots, first effect set, reverb, spatial engine) | **PARTIAL** -- 3 layer insert + 4 master slots **IMPLEMENTED**, with 10 real algorithms (Saturation, Chorus, TapeDelay, NodeDelay, FreqShiftEcho, FractalEcho, Reverb, Eq, Compressor, Limiter -- the master spec's "first effect set" basics now covered); bitcrush/wavefold/ensemble/flanger/phaser/diffusion delay and the spatial engine still PLANNED. See FX_BANK.md |
| 12 | MSEG / sequencer | **PARTIAL** -- arpeggiator **IMPLEMENTED** (`pw8/sequencer/Arpeggiator.hpp`, see ARPEGGIATOR.md); MSEG/step-sequencer-as-mod-source still PLANNED |
| 13 | Patch format productionization (schema, migrations, metadata) | **PARTIAL** -- schema v1 complete and hardened against untrusted input; migration *mechanism* exists with nothing to migrate yet (only one schema version so far) |
| 14 | Python API productionization | **PARTIAL** -- see PYTHON_API.md coverage table |
| 15 | Patchwork integration (Sound IR compilation boundary) | **PARTIAL** -- CLI/Python/schema boundaries exist and work; `EightPatchCompiler` (IR -> Patch) itself is PLANNED, see PATCHWORK_INTEGRATION.md |
| 16 | Plugin (VST3, AU, Standalone) | **PARTIAL, build-verified** -- builds against real JUCE 8.0.6; AU passes `auval` in full; `pluginval` passes at strictness 5 (max) on VST3 and AU; 270 parameters (macros, filter, LFO, all 8 operators, envelope, gain/pan, all 7 FX slots, arpeggiator) live-automatable end to end (`juce::AudioProcessorValueTreeState`); Standalone launches cleanly; no signature UI or real-DAW host-matrix yet. See PLUGIN_ARCHITECTURE.md |
| 17 | UI | **PARTIAL** -- PLAY mode **IMPLEMENTED** (the OBSIDIAN skin, `createEditor()` returns a real custom editor); DESIGN/LAB modes and the other 9 named skins PLANNED. See UI.md |
| 18 | AI features (Generate, Mutate, Breed, Lock) via Patchwork | **PLANNED** -- metadata hooks (`LockFlags`, `lineage`, deterministic seeding) exist; the AI pipeline itself lives in Patchwork. Also see docs/MCP_AND_NL_PATCH_GENERATION.md: its MCP server (Part A, `mcp_server/`) is **built and smoke-tested**; the natural-language "make me a laser sound" chat box (Part B) is still IDEA, not committed |
| 19 | Factory bank (512-1024 curated presets) | **PLANNED** -- 7 engineering test patches exist (`content/presets/`), not factory-curated content |
| 20 | Production hardening (host matrix, pluginval, auval, VST validator, fuzz tests, soak tests, perf optimization) | **PARTIAL** -- `auval` passes in full (see Phase 16); `murmur-fuzz-render` implemented and run (5,000 patches, 0 failures); `pluginval`, host matrix, and soak testing still PLANNED |

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

## Competitive check: Serum 2, Phase Plant & Zebra 3

See [COMPETITIVE_ANALYSIS.md](COMPETITIVE_ANALYSIS.md) for the full feature-parity
matrix. Summary: nearly every gap identified (modulator count, oscillator variety,
filter breadth, effects breadth, UI) was already tracked under an existing PLANNED
phase above -- this research mostly validated the existing phase scope rather than
changing it. The one exception: wavetable mip-mapping (Zebra's continuous splines
and Serum's smooth interpolation both solve the aliasing problem our Phase 2
wavetable oscillator hadn't yet) was bumped up and implemented in this same pass --
see "Follow-up pass: wavetable mip-mapping" below. The algorithm graph, deterministic
rendering, and native headless rendering remain genuine differentiators none of the
three competitors have. Per the master spec's explicit prohibition on copying UI
layout/visual identity from commercial products, no UI design or code resulted from
this pass -- only a feature/gap comparison.

## Follow-up pass: what "build it all" added

A second pass built and verified everything that was previously scaffolded or
merely described:

- `benchmarks/` -- real Google Benchmark suite (oscillators, algorithm graph, voice,
  full-patch render at the master spec's exact 44.1/48/96 kHz x 1/8/16/32-voice
  matrix), built and run.
- `tools/fuzz_render/` (`murmur-fuzz-render`) -- implemented and run: 5,000
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
- `murmur-fuzz-render` extended to randomize filter/LFO/mod-route parameters
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

## Follow-up pass: wavetable mip-mapping (Phase 2 -> DONE)

Closes the one concrete gap the Serum/Zebra research surfaced:

- `pw8::dsp::fft` -- a from-scratch textbook radix-2 Cooley-Tukey FFT/IFFT
  (`pw8/dsp/Fft.hpp`), control-path only.
- `oscillator::WavetableTable` -- multi-mip table data model with
  `viewForFrequency(freq, sampleRate)`, a cheap (<=16-comparison) per-sample-safe
  scan that picks the highest-fidelity mip that won't alias at the current note and
  actual output sample rate.
- `murmur-wavetable-builder` now generates real mip chains via FFT harmonic truncation
  (schema v2: a `mips` array, each with `maxHarmonic` and its own frame data).
- **Measured proof, not just plausible design**: `tests/unit/WavetableTableTests.cpp`
  synthesizes a harmonically rich table, renders the same high note through both the
  mip-selected path and the raw full-bandwidth mip, FFTs both outputs, and requires
  the mip-selected render's out-of-band spectral energy to be less than half the
  un-mipped render's -- passing with real margin.
- The full pipeline is wired end to end for the first time: `render::Engine::loadPatch()`
  now actually loads referenced wavetable files (`OperatorPatch::wavetableId`,
  currently resolved as a filesystem path -- see docs/PATCH_FORMAT.md "Wavetable
  Resource Resolution" for why that's still PARTIAL) rather than always rendering
  silence on the Wavetable engine.
- A new mod destination, `OperatorWavetablePosition`, and a real factory table +
  preset (`content/wavetables/basic_harmonic.json`, `content/presets/wt-morph.murmur`)
  prove it audibly: an LFO continuously sweeps the wavetable frame position.
- 17 new tests (FFT correctness/robustness, mip-selection logic, the aliasing-
  reduction proof, wavetable JSON loader roundtrip/robustness) -- 67 total, all
  passing. `murmur-fuzz-render` batch (3,000 patches) and a full plugin rebuild +
  `auval` re-validation both confirmed clean after the change.

## Follow-up pass: Arpeggiator (Phase 12 -> PARTIAL)

Answers "does it have an arpeggiator?" with a real implementation, not a roadmap
promise:

- `sequencer::Arpeggiator` -- performance-wide, up to 64 steps, 7 modes (Up/Down/
  UpDown/DownUp/AsPlayed/Random/Chord), free or tempo-synced rate, per-step gate/
  probability/ratchet/tie/velocity-scale/accent/octave-offset, latch. Two
  independently-cycling counters (step pattern, note sequence) give correct
  polymetric behavior when pattern length and chord size don't share a factor.
- Wired into `render::Engine`: when enabled, incoming MIDI feeds the arpeggiator
  instead of triggering voices directly, and the arpeggiator's output is fed back
  through the exact same note-trigger path a performer's MIDI would use -- no
  separate/parallel voice-triggering logic to keep in sync.
- Two real design bugs (tie's note-off timing, ratchet's sub-hit scheduling and
  note selection) were caught by tracing the sample-accurate timeline before
  writing tests, not by a failing test -- see ARPEGGIATOR.md for the detail.
- 10 new unit tests plus a render-level regression test that counts actual
  amplitude onsets in rendered audio (1 onset without the arp vs. 16 with it,
  exactly matching an 8 Hz/2s prediction) -- 78 total, all passing.
- `content/presets/arp-pluck.murmur` -- a non-uniform 8-step pattern (accent, a
  ratcheted double-hit, a deliberate rest) rather than a plain up-arp, proving the
  per-step modifiers actually compose.
- Full cross-build verification: all 4 configs (dev/benchmarks/python/plugin)
  clean, `murmur-fuzz-render` (1,500 patches) zero failures, `auval` re-validated.
- See [ARPEGGIATOR.md](ARPEGGIATOR.md) for full design detail and what's still
  PLANNED within Phase 12 (MSEG/mod-sequencer, arp-output MIDI channel handling).

## Follow-up pass: FX bank (Phase 11 -> PARTIAL)

Researched three named commercial delay/echo plugins (ChowMatrix, Cocoa Delay,
ValhallaFreqEcho -- architecture and feature set only, never code/UI, the same
boundary held for the Serum/Phase Plant/Zebra 3 research above) and used that
research to design and implement a real, multi-algorithmic FX bank, plus one
algorithm invented specifically for this project. Full detail in
[FX_BANK.md](FX_BANK.md); summary:

- `effects::EffectSlotParams` / `EffectChain` (`pw8/effects/`): 3 layer insert
  slots (`LayerPatch::insertEffects`) + 4 master slots (`Patch::masterEffects`),
  matching the master spec's slot counts. Each slot switches at runtime between
  6 real algorithms: **Saturation** (drive-normalized tanh), **Chorus**
  (modulated feedforward delay), **TapeDelay** (Cocoa-Delay-informed: wow/
  flutter drift, feedback saturation, ducking, Static/PingPong/Circular pan),
  **NodeDelay** (ChowMatrix-informed: a fixed-capacity 6-node tree of delay
  lines where a node's input can be another node's *output*, plus deterministic
  "Insanity" delay-time wander), **FreqShiftEcho** (ValhallaFreqEcho-informed: a
  single-sideband frequency shifter placed inside the delay's feedback loop, so
  repeats drift progressively out of tune), and **FractalEcho** -- this
  project's own invented algorithm: a procedurally-generated, seed-driven,
  self-similar delay tree, continuously morphable between two generated
  topologies via real-time coefficient-space interpolation (not a signal-domain
  crossfade). See FX_BANK.md "The invented algorithm" for the full case for why
  this specific combination is genuinely new.
- `dsp::HilbertTransformer`/`dsp::FrequencyShifter` (`pw8/dsp/HilbertTransformer.hpp`):
  a from-scratch FIR-based single-sideband frequency shifter. Its first
  implementation (a cascaded-allpass design using a remembered published
  coefficient table) was caught broken by direct FFT-based measurement before
  shipping -- see FX_BANK.md "The FrequencyShifter primitive" for the full story
  and the general lesson (measure a DSP block's actual frequency-domain
  behavior, don't trust a remembered coefficient table).
- 16 new unit tests (`tests/unit/EffectsTests.cpp`) plus 2 new render-level
  regression tests (a master TapeDelay slot turning one hit into several
  measured amplitude onsets; a layer insert Saturation slot measurably lowering
  peak) -- 96 total, all passing.
- 3 new engineering presets (`fx-node-tree.pw8`, `fx-fractal-morph.pw8`,
  `fx-freq-echo.pw8`), one of which (`fx-node-tree.pw8`) also demonstrates a
  layer insert slot and a master slot composing in the same patch.
- Full cross-build verification: dev/benchmarks/python/plugin all clean,
  `murmur-fuzz-render` (1,500 patches) zero failures, `auval` re-validated.

## Follow-up pass: plugin parameter automation (Phase 16, continued)

Closes the plugin's biggest gap between "builds and passes `auval`" and
"usable in a real DAW session": nothing was host-automatable before this pass.

- `MurmurProcessor::apvts` (`juce::AudioProcessorValueTreeState`, built
  from `plugin::createParameterLayout()`): 8 `AudioParameterFloat`s, one per
  macro, matching the master spec's "8 routable macros" surface.
- Made it actually *work*, not just publish: `render::Engine::setMacroValue()`
  writes straight into every active voice's live per-sample-read
  `macroValues` array, so a DAW automating a macro mid-note-hold audibly
  changes a currently-sustaining voice rather than only affecting the next
  one -- proven by a new Engine-level unit test
  (`tests/unit/EngineMacroLiveUpdateTests.cpp`) that measures RMS drop/
  recovery on a still-held voice as the macro is muted and restored.
  `processBlock()` pushes all 8 parameters into the running Engine every
  block via cached `std::atomic<float>*` pointers.
- Saved sessions round-trip the macros' *current* (possibly host-automated)
  values, not just a preset's original defaults (`getStateInformation()`/
  `loadPatch()` sync in both directions against the single `currentPatch_`
  source of truth) -- see docs/PLUGIN_ARCHITECTURE.md "Automation".
- Installed and ran `pluginval` (not previously run in this repo) at
  `--strictness-level 5`, the maximum: **SUCCESS on both the VST3 and the
  AU**, including its Automation, Plugin state, and Automatable Parameters
  suites -- a materially stronger signal than `auval` alone (which is
  AU-specific and doesn't exercise `pluginval`'s block-size-varying,
  automation-under-processing scenarios). `auval` itself now also exercises
  real parameter tests (`Checking parameter setting`/
  `Checking ramped parameter scheduling`) that were skipped entirely when
  there were zero published parameters.
- 1 new Engine-level unit test -- 97 total, all passing. All 4 build configs
  (dev/benchmarks/python/plugin) rebuilt clean.
- See [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md) "Automation" and
  "pluginval" for full detail, and "What's still missing" for what this pass
  deliberately didn't cover (real DAW host-matrix testing, `pluginval` in CI,
  automation beyond the 8 macros).

## Follow-up pass: every automatable parameter (Phase 16, continued -- 8 -> 270 parameters)

Per explicit user direction ("every param should be automatable"), the
automation surface above was deliberately widened from 8 macros to every
scalar field that's both POD-safe and currently audible on Layer A -- 270
parameters total. Full detail in [PLUGIN_ARCHITECTURE.md](PLUGIN_ARCHITECTURE.md)
"Automation" (including the complete, reasoned list of what's still
deliberately excluded and why); summary:

- `render::Engine` gained a full "Live parameter API": `setFilterLive`/
  `setLfoLive`/`setOperatorLive` (x8)/`setAmpEnvelopeLive`/`setLayerGainLive`/
  `setLayerPanLive`/`setMasterGainLive`/`setInsertEffectLive` (x3)/
  `setMasterEffectLive` (x4)/`setArpeggiatorScalarLive`, each POD-only (never
  touches a `std::string` field) and audio-thread safe, matching the pattern
  `setMacroValue()` already established.
- Two real correctness issues were caught and fixed while wiring this, not
  after: (1) `Arpeggiator::configure()` resets held-note/pattern state, so a
  new non-resetting `setLiveParams()` had to be added, or automating the
  arp's rate would have silently stopped it from ever playing; (2) effect
  slots' `nodes[]`/seed fields (not exposed to automation) had to be
  preserved via read-modify-write in `processBlock()`, or every block would
  have overwritten a NodeDelay/FractalEcho patch's actual topology with
  defaults.
- `plugin/src/state/PluginState.h`/`.cpp`: a field-spec-table-driven
  `createParameterLayout()` (not 270 hand-written near-duplicate blocks) --
  `ParamFieldSpec` tables for operators/filter/LFO/envelope/effect-slot/arp
  fields, looped over 8 operators and 7 FX slots.
- 4 new Engine-level unit tests (`tests/unit/EngineLiveParamsTests.cpp`):
  filter cutoff closing mid-hold measurably darkens a still-ringing voice,
  muting an operator's level mid-hold measurably silences it, an insert
  effect's saturation mid-hold measurably compresses a loud signal, and an
  arpeggiator rate change mid-pattern doesn't stop it from playing -- 101
  total tests, all passing.
- `auval` confirms exactly 270 published parameters and still passes in
  full; `pluginval --strictness-level 5` re-run and re-confirmed SUCCESS on
  both the VST3 and the AU at the full 270-parameter count (not a stale
  result carried over from the 8-macro version).
- All 4 build configs (dev/benchmarks/python/plugin) rebuilt clean.

## GPU acceleration (researched, not adopted)

Per user request, researched GPU Audio (the company), NVIDIA VRWorks Audio, and AMD
TrueAudio Next -- see [GPU_ACCELERATION_RESEARCH.md](GPU_ACCELERATION_RESEARCH.md).
This repository stays CPU-only: a hard CUDA dependency would break the portability
this project already commits to (Apple Silicon, embedded/ARM, Linux render-farm
nodes without NVIDIA GPUs), the real difficulty (a realtime-deadline-aware GPU
scheduler bridging the SIMD-batch/MIMD-realtime mismatch) is nontrivial systems
engineering rather than a quick win, and current CPU headroom is enormous (32-voice
96kHz full-patch render at ~16ms per second of audio, in an unoptimized Debug
build). No roadmap phase was added or changed; this is a decision record for future
reference, most relevant to the PLANNED Additive/Granular/Resonator engines and
Phase 11 reverb if pursued later.

A related but distinct question -- GPU-accelerated *UI rendering* (spectrum
analyzer, oscilloscope, wavetable preview via `juce::OpenGLContext`, no CUDA/compute
dependency) -- was also researched and architected, documented (not implemented) in
`docs/PLUGIN_ARCHITECTURE.md` "Visualization"; it stays under Phase 17 like the rest
of the UI.

## GATE 4: the acceptance patch (PASSED)

A later, more detailed product brief for this project introduced an explicit
sonic acceptance gate before further feature work: build a
"MASSIVE DARK EVOLVING METALLIC BASS" using only what's *actually implemented*
today, and if it's mediocre, stop adding features and fix fundamentals
instead. `content/presets/gate4-massive-dark-metallic-bass.murmur` is that patch,
built entirely from real, already-shipped DSP -- no dedicated unison engine
(not DSP-wired yet, Phase 7) and no dedicated FM/Resonator engine type
(silent, Phase 10 PLANNED) were needed:

- **MASSIVE**: 3 hand-detuned Classic saws (ratios 0.996/1.0/1.004 -- the same
  fake-unison technique `wide-saw.pw8` already established) plus two centered
  sub sines (ratio 0.5 and 0.25) for weight without losing the fundamental.
- **DARK**: Filter 1 lowpass at 350Hz, resonance 0.35.
- **METALLIC**: a genuine `RingMod` algorithm-graph edge -- a non-output
  3.17-ratio triangle (node 0) ring-modulates a dedicated, low-level
  1.5-ratio saw (node 1), so the inharmonic ring-mod texture is a controlled
  layer, not applied to the fundamental (which would have gutted the bass
  weight at the modulator's zero-crossings).
- **EVOLVING**: LFO1 -> FilterCutoff (slow sweep) and LFO1 ->
  OperatorLevel(node 1) (the metallic voice swells in and out), plus
  Velocity -> FilterCutoff for a hit-hard response.
- A single Saturation insert (10dB drive) adds harmonic weight, matching the
  brief's "should sound exceptional before external mastering" bar --
  deliberately no master reverb/width processing to disguise the core tone.

**Verified quantitatively, not just by ear** (rendered against
`content/test_midi/bass-line.mid`, a real walking bassline, not a single
static tone): peak 0.949 / RMS 0.345, no clipping, DC offset negligible
(0.0035), zero NaN/Inf. A windowed-FFT analysis (4096-sample windows, 50%
overlap) confirms the design claims rather than just asserting them: mean
low-frequency (<500Hz) energy ratio 0.578 (dark/bass-weighted), and the
high-frequency (>2kHz) energy ratio's variation over time (std/mean = 0.85,
well above a 0.15 "is this actually swelling" bar) confirms the metallic
texture voice's LFO-driven swell is real and audible, not just present.

**Conclusion**: the current oscillator/filter/algorithm-graph/mod-matrix/FX
primitives are of sufficient quality to build a genuinely good patch -- GATE 4
passes. This validates proceeding to the next phase (deeper modulation)
rather than reworking fundamentals first.

## GATE 5: full modulation foundation (Phase 5, PARTIAL -> IMPLEMENTED)

Per user direction following GATE 4 ("DO it"), expanded from the 1-envelope/
1-LFO/VOICE-scope-only version to the master spec's full target: 8 envelopes, 8
LFOs, and VOICE+LAYER/GLOBAL mod scope. Full design rationale in
[MODULATION.md](MODULATION.md); summary:

- `voice::Voice` now owns 8 independent `DahdsrEnvelope` instances and 8
  independent `Lfo` instances (`envelopes[0]`/`lfos[0]` keep the exact behavior
  the single-instance version had -- driving the VCA and voice lifetime -- the
  other 7 of each are equally general-purpose mod matrix sources).
- **LAYER/GLOBAL scope is real for LFOs**, not just recorded and ignored:
  `render::Engine` ticks a separate, shared bank of 8 `Lfo` instances once per
  sample (before the voice loop), so a LAYER-scoped route reads one value
  identical across every voice that sample, regardless of when each note
  started -- proven end-to-end by triggering the same note at two different
  times and showing the resulting pan differs by the LFO's *absolute* elapsed
  phase, not each voice's own note-relative phase
  (`tests/regression/RenderSanityTests.cpp`). Envelope LAYER/GLOBAL scope is a
  deliberate non-goal (no single coherent trigger point for a layer-wide
  envelope with 0-32 independently overlapping notes) -- documented, not a gap.
- `ModSource` grew from 15 to 29 values (`Lfo1`-`Lfo8`, `Env1`-`Env8`, 4
  performance sources, `Macro1`-`Macro8`).
- Schema bumped to **v2** with a real migration step for the first time in this
  project (previously a documented no-op) -- see PATCH_FORMAT.md "Migration".
  Two things needed migrating: `LayerPatch`'s singular `ampEnvelope`/`lfo1`
  fields becoming 8-slot arrays, and (a real bug caught before shipping, not
  hypothetical) every `modRoutes[].source` ordinal shifting when the new enum
  values were inserted -- without remapping, a v1 preset's `AmpEnvelope`/
  `Velocity` routes would have silently resolved to the wrong new source.
  Caught by inspecting this project's own shipped presets (`dark-bass.pw8`,
  `wide-saw.pw8`) before considering the pass done, not by a user report.
- Plugin automation extended from 270 to **361 parameters**: all 8 LFOs (40)
  and all 8 envelopes (64) replacing the single-instance versions (5 and 8).
  `auval` confirms 361 published parameters; `pluginval --strictness-level 5`
  re-confirmed SUCCESS on both VST3 and AU at the new count.
- 6 new tests (2 ModMatrixExecutor scope tests, 1 LAYER-scope render regression
  test, 2 migration tests, 1 fuzz-tool coverage extension randomizing all 8
  envelopes/LFOs and the full 29-source/3-scope space) -- 107 total, all
  passing. `murmur-fuzz-render` (1,500+ patches across batches) zero failures.
- All 4 build configs (dev/benchmarks/python/plugin) rebuilt clean; Python
  bindings' minimal envelope API repointed at `envelopes[0]` (unchanged
  behavior, same PARTIAL API-surface scope as before -- exposing all 8
  envelopes/LFOs to Python remains a separate future PYTHON_API.md item).

## GATE 10: Reverb, EQ, Compressor, Limiter (Phase 11, continued)

Per user direction, closed the FX bank's remaining "first effect set" gap --
the four basics every synth needs that the initial FX bank pass (6 algorithms:
Saturation/Chorus/TapeDelay/NodeDelay/FreqShiftEcho/FractalEcho) didn't cover.
Full design detail in [FX_BANK.md](FX_BANK.md) "GATE 10"; summary:

- **Reverb**: a 4-line Householder-matrix FDN (Jean-Marc Jot-style), pre-delay,
  per-line damping, RT60-derived decay.
- **Eq**: 3-band parametric (low shelf/mid peak/high shelf), RBJ Audio EQ
  Cookbook biquad formulas (`dsp::Biquad`, new).
- **Compressor**: feedforward, stereo-linked peak detection, quadratic soft
  knee, dB-domain attack/release smoothing.
- **Limiter**: true lookahead (sliding-window-minimum gain over a ring
  buffer) -- provably no overshoot above the ceiling, not just a fast
  compressor.
- A real bug caught before shipping: Reverb's pre-delay at 0ms produced
  ~200ms of total silence instead of near-zero pre-delay, because
  `dsp::DelayLine`'s read-before-write convention means a delay of *exactly*
  0 samples reads data from a full buffer-length ago. Caught by this effect's
  own render-level test (expected a decaying tail, measured literal silence)
  before it ever reached a preset. Fixed by flooring pre-delay at 1 sample,
  the same floor every other delay-based effect in this codebase already
  carries for exactly this reason.
- `EffectSlotParams` grew from 23 to 43 scalar fields (adding the 4 new
  algorithms' parameters); plugin automation grew from 361 to **501
  parameters** as a direct consequence (43 fields x 7 FX slots = 301, up from
  161). `auval` confirms 501 published parameters; `pluginval
  --strictness-level 5` re-confirmed SUCCESS on both VST3 and AU.
- `murmur-fuzz-render`'s `randomPatch()` now randomizes all 7 `EffectSlotParams`
  slots for the first time (closing a gap that had persisted across the
  arpeggiator, original FX bank, and GATE 5 passes) -- 1,000 fully-randomized
  patches (seed 13), zero failures.
- 9 new unit tests + 2 new render-level regression tests -- 118 total, all
  passing.
- `content/presets/fx-master-chain.murmur` -- all 4 new algorithms arranged
  across the master bus's 4 slots the way a real mix would use them
  (Eq -> Reverb -> Compressor -> Limiter).

## GATE 11: Reverb redesign -- "nuanced and massive" (Phase 11, continued)

Per explicit user direction ("research Bricasti M7 and make Reverb algos
adhere to these principles. It needs to be nuanced and massive"), GATE 10's
Reverb -- deliberately the simplest correct FDN, built to prove the
integration end to end -- was redesigned from the ground up. Full design
detail and research citations in [FX_BANK.md](FX_BANK.md) "GATE 11"; summary:

- The late tank grew from 4 to 8 Householder-mixed FDN lines (CloudSeed's
  documented "many delay lines for density/massiveness" approach), fed through
  a new Schroeder/Dattorro-style 4-stage series-allpass input diffuser
  (`reverbDiffusion`/`reverbDensity`, two genuinely distinct controls: per-
  stage smoothness vs. how many stages are engaged).
- **Frequency-dependent (multiband) decay** -- the single most distinctive
  researched principle, informed by (not ported from) the actual Bricasti M7
  owner's manual (Rev 5.02.08, fetched and read directly): independent HF/LF
  RT60 *multipliers* of a mid-band `reverbDecaySeconds`, each with its own
  crossover (`reverbHighRatio`/`reverbHighCrossoverHz`/`reverbLowRatio`/
  `reverbLowCrossoverHz` -- LF ratio can exceed 1, i.e. bass can ring *longer*
  than mid, matching the M7's own 0.2-4.0x range), realized per Jot's
  published FDN "absorptive filter" technique as a low-shelf + high-shelf
  `dsp::Biquad` pair per line around the flat mid-band gain.
- Per-line, per-line-decorrelated late-tank delay-length modulation
  (`reverbModDepth`/`reverbModRateHz`) -- the M7's "Reverb Modulation," the
  mechanism that keeps a dense multi-line network smooth rather than
  metallic.
- A separate, parallel, independently-leveled early-reflection tap cluster
  (`reverbEarlyLevel`/`reverbLateLevel`), matching the M7's explicit early/
  late engine split.
- `reverbSizeParam` explicitly decoupled from decay time; new
  `reverbRollOffHz` (output lowpass) and `reverbVlfCutDb` (low-shelf cut,
  -18 to 0dB) round out the M7's "Roll Off" and "VLF Cut".
- A real research correction caught before it reached any doc or code: an
  initial web search surfaced a plausible but *wrong* claim that the M7 has
  "Spin"/"Wander" modulation controls -- those are actually Lexicon's terms.
  Fetching and reading the real M7 manual directly (not a secondhand summary)
  caught this before it was cited anywhere.
- `EffectSlotParams`'s Reverb field count grew from 4 to 15 (`reverbDampingHz`
  retired -- still read from old documents for backward compatibility via key
  presence inside `EffectSlotParams`'s own JSON defaulting, no schema version
  bump needed, the same kind of per-field compatibility decision GATE 10 made
  growing 23 to 43 fields). `EffectSlotParams` overall: 43 -> 54 fields; plugin
  automation: 501 -> **578 parameters** (54 fields x 7 FX slots = 378, up from
  301). `auval` confirms 578 published parameters; `pluginval
  --strictness-level 5` re-confirmed SUCCESS on both VST3 and AU.
- `dsp::Biquad` gained a `setLowpass` method (RBJ Cookbook, reused for both
  Roll Off and, indirectly, the absorption-filter math above).
- 7 new unit tests, each measuring one of the redesign's specific claims
  directly (multiband decay's high/low energy ratio shift, diffusion/density's
  crest-factor reduction, modulation stability at maximum depth/rate over a
  20-second decay, early/late independence, VLF Cut/Roll Off's targeted-band
  attenuation, and Size/decay decoupling) -- 125 total tests, all passing.
- `murmur-fuzz-render` -- 1,000 fully-randomized patches (seed 14) now exercising
  Reverb's full new 15-field shape, zero failures.
- `content/presets/fx-master-chain.murmur`'s Reverb slot updated to demonstrate
  the new capability (extended low-frequency ring, faster high-frequency
  decay, high diffusion/density, gentle late-tank modulation).

## UI GATE 1: PLAY mode, the OBSIDIAN skin (Phase 17)

Per explicit user direction ("how do we make the UI amazing" -> "1 skin but it
has to be amazing" -> "do it"), built PLAY mode's first real screen: a custom
`juce::AudioProcessorEditor` (`ui::PlayModeEditor`) replacing
`juce::GenericAudioProcessorEditor`, in one fully-realized skin (OBSIDIAN, the
safest of the master spec's 10 named skins for "genuinely premium") rather
than scaffolding several. Full design writeup, architecture, and a real
geometry bug caught and fixed via `lldb` (not guesswork) during this pass:
see [UI.md](UI.md). Summary:

- Custom `ObsidianLookAndFeel` + `ObsidianPalette` (one file, every color
  token) -- knobs are flat, precise, glow-arc rotaries, not skeuomorphic chrome.
- `AlgorithmGraphView`: the centerpiece, a read-only rendering of Layer A's
  live 8-node graph with nodes fixed on a circle (never a draggable modular
  patcher -- the master spec's "no patch-cable spaghetti" rule satisfied
  architecturally) and `EdgeType`-colored curved edges with an animated
  signal-flow pulse; self-feedback loops and not-yet-implemented engine types
  (dashed ring) both render distinctly. Verified against a real patch with
  edges (`fm-bell.pw8`), not just the edge-less default init patch.
  `MurmurProcessor` gained a message-thread-only `getCurrentPatch()`
  accessor for this.
- `MacroStrip`, `FilterLfoPanel`, `FxChainStrip`, `PatchBrowserBar` -- the
  rest of PLAY mode's one screen, all live-wired to real APVTS parameters via
  `SliderAttachment`/`ButtonAttachment`.
- `pluginval --strictness-level 5` re-confirmed SUCCESS on both VST3 and AU
  against the *real* custom editor (its Editor/Editor Automation suites are
  what actually exercise a custom editor's paint/resize discipline under
  automation, unlike `auval` alone).
- A real bug, not hypothetical: an initial layout budget starved the FX chain
  strip down to ~10px of height, driving the knob-painting math's derived
  radii negative -- caught via `lldb`, conditionally breaking on
  `juce_GraphicsContext.cpp`'s internal bounds check to get the exact call
  site and dimensions rather than guessing. Fixed both the specific layout
  (rebalanced so utility strips get fixed generous heights and the graph
  panel takes the remainder) and defensively (the LookAndFeel now floors its
  radius math so a future layout mistake can't reproduce the failure mode).

DESIGN and LAB modes, the other 9 named skins, and GPU-accelerated visuals
(spectrum/scope) all remain PLANNED -- see UI.md "What's PLANNED".

## UI GATE 2 & 3: visual overhaul, then graph interactivity + drag-to-modulate

Two more PLAY-mode passes, both driven by direct user/mockup feedback rather
than committed scope; full writeups (including "A real bug" for GATE 2, none
for GATE 3) live in [UI.md](UI.md), not duplicated here:

- **GATE 2** ("make it way radder", against a denser reference mockup):
  typography (`ObsidianFonts.h`), a deliberate cyan/amber duotone, real card
  drop-shadow depth, a subtle background texture/vignette, ambient
  breathing-glow "life" on the graph's output node, and a proper wordmark
  header -- visual language only, no new feature surface.
- **GATE 3** ("why can't I click on other oscillators" / "I dont understand
  the graph"): the algorithm graph became clickable (`OperatorEditorPanel`
  shows whichever node is selected), gained a plain-language caption + edge
  color legend, and PLAY mode gained real drag-to-modulate (3 `ModSourceChip`s
  -- LFO 1/amp envelope/Velocity -- onto Filter Cutoff/Resonance, backed by a
  new lock-free live mod-route path, `Engine::setModRoutesLive()`, that
  reaches an already-sustaining voice the next sample rather than needing a
  re-trigger). Also fixed real DAW-usage friction found along the way: a
  `PatchBrowserBar` "Load..." button, since there was previously no way to get
  a saved `.pw8` into a running plugin instance inside an actual host.

`auval` and `pluginval --strictness-level 5` re-confirmed SUCCESS on VST3 and
AU after both passes (578 published parameters unchanged throughout -- GATE 2
was paint-only, and GATE 3's mod-route live-editing path deliberately stays
outside the host-automation surface, same as the mod-route list always has
been). 126 tests total, all passing (+1 from GATE 3's
`Engine::setModRoutesLive` coverage).

## GATE 12: Rebrand to MURMUR (repo, file extension, tooling)

Not a DSP/UI pass -- infrastructure/branding work, kept in this file for the same
reason every other gate is: it's the project's running log of major completed/in-progress
efforts. Full decisions record, real inventory counts, and step-by-step sequencing in
[REBRAND_MURMUR.md](REBRAND_MURMUR.md). Status: IN PROGRESS (Steps 1-3 of that doc --
inventory, extension-handling code, cosmetic/identifier renames -- are done; mass preset
rename, CLI tool renames, full verification, and the repo rename itself have not
started).

Summary: the plugin's own bundle identity (`com.patchwork.murmur`, `PRODUCT_NAME
"MURMUR"`) has been correct since the first real release, `v1.0.0` -- verified via
`git show v1.0.0:plugin/CMakeLists.txt`, not touched by this gate. What was still
old-branded, now fixed in Steps 1-3: this repo's own CMake `project()` name
(`patchwork_eight` -> `murmur`), the `PatchworkEightProcessor` class (-> `MurmurProcessor`,
150+ dependent files, compiler-verified), the Python binding module
(`patchwork_eight` -> `murmur`, compiler-verified), and the docs/prose pass. Still
pending: the repo's own name on GitHub (`patchwork-eight` -> `murmur-app`), the
`.pw8` patch extension (dual-read code already lands in Step 2; the actual mass rename of
the 1,158 files themselves is Step 4), the `pw8-` prefix baked into those files'
`metadata.id` field (same Step 4), and the `pw8-*` CLI tool names (Step 5). The
internal C++ `pw8::` namespace and `pw8_core`/`pw8_plugin` CMake target names are a
**deliberate, permanent non-change** (401 files, purely internal, no user-facing value)
-- documented so a future pass doesn't "finish the job" by accident.

## Immediate next steps (suggested, not committed)

1. A real DAW host-matrix pass (Ableton, Logic, Reaper, Bitwig, etc.) -- `auval`
   and `pluginval` at max strictness are both green now, and UI GATE 3's
   `PatchBrowserBar` "Load..." button already closed one piece of real friction
   found using REAPER for the first time; a fuller pass across the rest of the
   matrix is still the natural next increment toward a genuinely shippable
   plugin rather than a from-scratch effort.
2. Unison DSP wiring -- the schema (`UnisonSettings`) and every acceptance patch
   built so far (`wide-saw.pw8`, `gate4-massive-dark-metallic-bass.pw8`) fake it
   via hand-detuned operators; real unison is the single biggest remaining gap
   for the "MASSIVE CENTER" sonic philosophy a later product brief called out.
3. Filter 2 (nonlinear character filter) -- Filter 1's TPT SVF proved the per-voice
   filter integration point; a second filter stage is now a smaller increment than
   it was before Filter 1 existed.
4. bitcrush/wavefold/ensemble/flanger/phaser/diffusion delay -- the FX bank now
   covers 10 algorithms including the full "first effect set"; these are the
   remaining items from the original `pw8/effects/README.md` scope.
5. Wavetable content-addressed resource resolution (replacing the current
   filesystem-path-as-`wavetableId` scheme) as part of the broader content pipeline.
6. Algorithm morph and dual-layer mixing (Phase 8/9) -- Layer B's full schema
   already round-trips; only the voicing/mixing/morph DSP is missing.
7. A full mod-matrix UI (all 29 sources, all 5 destinations, multi-route-per-
   destination authoring) -- UI GATE 3's drag-to-modulate deliberately covers
   only 3 sources x 2 destinations as PLAY mode's "quick assign" surface;
   everything else in a loaded patch is still visible (`ModSourceStrip`'s
   connections list reads every real route honestly) but only editable via a
   hand-authored `.pw8` today.
