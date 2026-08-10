# Roadmap

Status as of this repository's initial engineering pass. Legend: **DONE** / **PARTIAL** / **PLANNED**.

| Phase | Scope | Status |
|---|---|---|
| 0 | Repository foundation (structure, CMake, core library skeleton, tests, benchmarks, docs, CI, coding standards) | **DONE** |
| 1 | First sound (Engine, Voice, VoiceAllocator, ClassicOscillator, DAHDSR, MIDI note handling, stereo output, native renderer) | **DONE** |
| 2 | Wavetable (preprocessing, mipmapping, frame interpolation, wavetable source) | **DONE** -- oscillator, builder tool, and FFT-based mip-mapping/band-limiting all implemented and proven to reduce aliasing by a measured >2x (see DSP_ENGINE.md); a real factory table and preset (`wt-morph.pw8`) exercise the full pipeline end to end |
| 3 | 8-node algorithm graph (nodes, edges, validation, compiler, compiled execution, audio routing) | **DONE**, and beyond AUDIO-only -- all 7 edge types implemented |
| 4 | PM/FM/AM/RING typed modulation edges | **DONE** at the graph level (stretch goal achieved); a dedicated Engine Type 3 (FM/PM) with its own ratio/fixed-frequency modes is still PLANNED |
| 5 | Modulation (envelopes, LFO, mod matrix, macros, performance controls) | **PARTIAL** -- amplitude envelope, one per-voice LFO (6 waveforms, 4 modes incl. tempo-sync), and a VOICE-scoped mod matrix (6 sources incl. 8 macros, 4 destinations) are all IMPLEMENTED; 8-envelope/8-LFO/LAYER+GLOBAL-scope targets PLANNED |
| 6 | Filters (clean multimode, first character filter) | **PARTIAL** -- Filter 1 (TPT state-variable: LP/HP/BP/notch/peak, per-voice, mod-matrix-modulatable cutoff/resonance, key tracking) is **IMPLEMENTED**; Filter 2 (nonlinear character filter) PLANNED |
| 7 | Unison / stereo (unison, voice drift, pan, width, center gravity) | **PARTIAL** -- data model present (`UnisonSettings`, `centerGravity` on `LayerPatch`); DSP wiring PLANNED. `content/presets/wide-saw.pw8` demonstrates the *effect* today via hand-detuned operators rather than an automated unison engine |
| 8 | Dual layer (Layer A, Layer B, stack, layer morph) | **PARTIAL** -- schema complete (`LayerMode` enum, full `layerB` data), only `SINGLE_A` is actually voiced/rendered |
| 9 | Algorithm morph (same-topology, different-topology) | **PLANNED** |
| 10 | Additional engines (additive, phase/shape, noise, resonator, granular) | **PLANNED** -- `EngineType` enum and dispatch points exist; each renders silence until implemented |
| 11 | FX (insert slots, master slots, first effect set, reverb, spatial engine) | **PARTIAL** -- 3 layer insert + 4 master slots **IMPLEMENTED**, with 6 real algorithms (Saturation, Chorus, TapeDelay, NodeDelay, FreqShiftEcho, FractalEcho); reverb/EQ/compressor/limiter and the rest of the original effect-set list still PLANNED. See FX_BANK.md |
| 12 | MSEG / sequencer | **PARTIAL** -- arpeggiator **IMPLEMENTED** (`pw8/sequencer/Arpeggiator.hpp`, see ARPEGGIATOR.md); MSEG/step-sequencer-as-mod-source still PLANNED |
| 13 | Patch format productionization (schema, migrations, metadata) | **PARTIAL** -- schema v1 complete and hardened against untrusted input; migration *mechanism* exists with nothing to migrate yet (only one schema version so far) |
| 14 | Python API productionization | **PARTIAL** -- see PYTHON_API.md coverage table |
| 15 | Patchwork integration (Sound IR compilation boundary) | **PARTIAL** -- CLI/Python/schema boundaries exist and work; `EightPatchCompiler` (IR -> Patch) itself is PLANNED, see PATCHWORK_INTEGRATION.md |
| 16 | Plugin (VST3, AU, Standalone) | **PARTIAL, build-verified** -- builds against real JUCE 8.0.6; AU passes `auval` in full; `pluginval` passes at strictness 5 (max) on VST3 and AU; 270 parameters (macros, filter, LFO, all 8 operators, envelope, gain/pan, all 7 FX slots, arpeggiator) live-automatable end to end (`juce::AudioProcessorValueTreeState`); Standalone launches cleanly; no signature UI or real-DAW host-matrix yet. See PLUGIN_ARCHITECTURE.md |
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

## Follow-up pass: wavetable mip-mapping (Phase 2 -> DONE)

Closes the one concrete gap the Serum/Zebra research surfaced:

- `pw8::dsp::fft` -- a from-scratch textbook radix-2 Cooley-Tukey FFT/IFFT
  (`pw8/dsp/Fft.hpp`), control-path only.
- `oscillator::WavetableTable` -- multi-mip table data model with
  `viewForFrequency(freq, sampleRate)`, a cheap (<=16-comparison) per-sample-safe
  scan that picks the highest-fidelity mip that won't alias at the current note and
  actual output sample rate.
- `pw8-wavetable-builder` now generates real mip chains via FFT harmonic truncation
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
  preset (`content/wavetables/basic_harmonic.json`, `content/presets/wt-morph.pw8`)
  prove it audibly: an LFO continuously sweeps the wavetable frame position.
- 17 new tests (FFT correctness/robustness, mip-selection logic, the aliasing-
  reduction proof, wavetable JSON loader roundtrip/robustness) -- 67 total, all
  passing. `pw8-fuzz-render` batch (3,000 patches) and a full plugin rebuild +
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
- `content/presets/arp-pluck.pw8` -- a non-uniform 8-step pattern (accent, a
  ratcheted double-hit, a deliberate rest) rather than a plain up-arp, proving the
  per-step modifiers actually compose.
- Full cross-build verification: all 4 configs (dev/benchmarks/python/plugin)
  clean, `pw8-fuzz-render` (1,500 patches) zero failures, `auval` re-validated.
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
  `pw8-fuzz-render` (1,500 patches) zero failures, `auval` re-validated.

## Follow-up pass: plugin parameter automation (Phase 16, continued)

Closes the plugin's biggest gap between "builds and passes `auval`" and
"usable in a real DAW session": nothing was host-automatable before this pass.

- `PatchworkEightProcessor::apvts` (`juce::AudioProcessorValueTreeState`, built
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
instead. `content/presets/gate4-massive-dark-metallic-bass.pw8` is that patch,
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

## Immediate next steps (suggested, not committed)

1. A real DAW host-matrix pass (Ableton, Logic, Reaper, Bitwig, etc.) -- `auval`
   and `pluginval` at max strictness are both green now, so this is the natural
   next increment toward a genuinely shippable plugin rather than a
   from-scratch effort.
2. Reverb, EQ, compressor, limiter -- the FX bank has 6 real algorithms now but
   none of the master spec's "first effect set" basics; see FX_BANK.md "What's
   PLANNED, not implemented".
3. Expand modulation to the full 8-envelope/8-LFO/LAYER+GLOBAL-scope target now that
   the 1-of-each VOICE-scoped version has proven the data model and execution
   pattern end to end.
4. Filter 2 (nonlinear character filter) -- Filter 1's TPT SVF proved the per-voice
   filter integration point; a second filter stage is now a smaller increment than
   it was before Filter 1 existed.
5. Wavetable content-addressed resource resolution (replacing the current
   filesystem-path-as-`wavetableId` scheme) as part of the broader content pipeline.
