# Modulation

## Envelopes

**IMPLEMENTED, 8 per layer.** `envelope::DahdsrEnvelope` (`pw8/envelope/DahdsrEnvelope.hpp`):
Delay/Attack/Hold/Decay/Sustain/Release, per-stage curve shape (0 = linear, higher =
more exponential-feeling), legato retrigger-from-current-level option.
`LayerPatch::envelopes` is now an 8-slot array (`core::kNumEnvelopesPerLayer`);
each `voice::Voice` owns 8 independent `DahdsrEnvelope` instances. `envelopes[0]` is
conventionally "the" amp envelope -- the only one wired directly to the VCA
(`Voice::renderSample`'s `amp = filtered * env * velocity * outputGain`) and to
voice lifetime (`Voice::isFree()`) -- but all 8 render every sample and are equally
selectable mod matrix sources (`ModSource::Env1`..`Env8`), e.g. a dedicated filter
envelope (`Env2 -> FilterCutoff`) alongside the amp envelope, entirely independent
shapes. `Voice::noteOff()` releases all 8 together, so envelopes 2-8 get a proper
release stage too rather than being stuck at sustain.

Automating an envelope's times (`Engine::setEnvelopeLive()`) takes effect on that
envelope's *next* note-on, not an already-ringing voice's current ramp --
`DahdsrEnvelope` captures its targets/coefficients once at `noteOn()` and has no
live mid-ramp-retarget API. See `tests/dsp/DahdsrEnvelopeTests.cpp` and
`tests/unit/EngineLiveParamsTests.cpp`.

**Not yet implemented:** envelope loop mode, tempo sync, LAYER/GLOBAL scope (see
"Scope" below for why the last one specifically isn't planned as a simple
extension).

## LFOs

**IMPLEMENTED, 8 per layer, VOICE + LAYER/GLOBAL scope.** `lfo::Lfo`
(`pw8/lfo/Lfo.hpp`): sine, triangle, saw, square, sample-and-hold, and
smooth-random waveforms; free, retrigger, one-shot, and tempo-sync modes (a fixed
table of musical divisions from 4 bars down to a dotted 1/4,
`lfo::kSyncDivisionQuarterNotes`). `LayerPatch::lfos` is an 8-slot array
(`core::kNumLfosPerLayer`, `ModSource::Lfo1`..`Lfo8`).

Two independent instantiations of each LFO index coexist, selected per-route by
`ModRoute::scope`:

- **VOICE scope** (default): each `voice::Voice` owns its own 8 `Lfo` instances,
  seeded deterministically per note (continuing the same seeded RNG stream that
  randomizes operator phase, so all 8 get decorrelated-but-reproducible seeds, not
  8 copies of one). Free-running LFOs across simultaneously-held notes are *not*
  phase-locked to each other -- each voice's LFO starts from its own note-on.
- **LAYER / GLOBAL scope**: `render::Engine` owns a separate bank of 8 `Lfo`
  instances (`layerLfosA_`), ticked once per sample in `Engine::process()` --
  *before* the voice loop, so every voice reads the identical value for that LFO
  index this sample, regardless of when each note started. Initialized once in
  `loadPatch()` rather than per-note (there's no single coherent "note-on" for a
  layer-wide modulator); `Retrigger`/`OneShot` modes have no distinct trigger event
  at this scope and behave the same as `Free`. LAYER and GLOBAL currently behave
  identically (only Layer A is voiced, Phase 8) -- true GLOBAL (shared across both
  layers) lands once dual-layer mixing does.
  Proven end-to-end, not just at the mixing-logic level: `tests/regression/RenderSanityTests.cpp`'s
  scope test triggers the same note at two different times and shows a LAYER-scoped
  Pan route's phase (and therefore the resulting pan) is the *same absolute* phase
  regardless of note-start time, while a VOICE-scoped route resets fresh at each
  note's own onset.

`Engine::setLfoLive(lfoIndex, params)` updates both the VOICE-scope per-voice
template/active-voice copies and the LAYER/GLOBAL-scope shared instance's params in
one call -- automating an LFO's rate/waveform/etc. affects both scopes
simultaneously, which is already everything a single host-automated LFO parameter
can meaningfully mean (see `docs/PLUGIN_ARCHITECTURE.md` "Automation").

Tempo sync reads the render/host BPM (`render::Engine::setTempo()`, threaded from
`RenderOptions::bpm` in the native renderer and from the JUCE host playhead in the
plugin). See `tests/dsp/LfoTests.cpp`,
`tests/regression/RenderSanityTests.cpp`'s BPM-tracking end-to-end test, and its
LAYER-scope test above.

## Mod Matrix

**IMPLEMENTED.** `modulation::ModMatrixExecutor` (`pw8/modulation/ModMatrixExecutor.hpp`),
executing `LayerPatch::modRoutes` (a `core::FixedVector<ModRoute, core::kMaxModRoutes>`,
capacity 64) once per voice per sample. No allocation; routes with `source == None`
or `destination == None` are skipped cheaply.

Sources (30 total): `Lfo1`..`Lfo8`, `Env1`..`Env8`, `Velocity`, `ChannelPressure`,
`PolyAftertouch`, `MpeSlide`, `Macro1`..`Macro8`, `ModWheel` (MIDI CC1).

Destinations: `FilterCutoff` (exponential/semitone-scaled), `FilterResonance`
(additive), `OperatorLevel` (multiplicative, per-node via `targetIndex`), `Pan`
(additive), `OperatorWavetablePosition` (additive, per-node via `targetIndex`,
result clamped 0..1 -- only meaningful for operators on the Wavetable engine).

### Scope

`ModRoute::scope` (`ModScope`: `Voice`/`Layer`/`Global`) is fully executed for LFO
sources (see "LFOs" above). For every other source type, a route's declared scope
is intentionally read but not acted on -- it still resolves to the per-voice value
regardless:

- **Envelope LAYER/GLOBAL scope is deliberately not implemented.** There's no
  single coherent trigger point for a "layer-wide envelope" the way there is for an
  always-running LFO: with 0-32 notes independently overlapping, "when does the
  layer's shared envelope start its attack" has no good answer (first note held?
  last note released? something in between?) without inventing new, non-obvious
  semantics. A per-voice envelope is unambiguous and is what every route actually
  gets today.
- **Macros are already global by construction** (one `Patch::macros[8]` array, not
  per-voice), so VOICE/LAYER/GLOBAL scope is moot for them.
- **Velocity/pressure/aftertouch/slide are inherently per-note performance data** --
  a "layer-wide velocity" isn't a meaningful concept the way a shared LFO clock is.

`tests/unit/ModMatrixTests.cpp` covers: neutral output with no routes, correct
Velocity->FilterCutoff scaling, multiplicative composition of multiple routes to the
same operator, inactive-route skipping, macro-index resolution, reading any of the
8 LFOs/envelopes by index, LAYER/GLOBAL scope reading the shared LFO tick instead of
the per-voice one, and envelope sources ignoring declared scope.
`content/presets/dark-bass.murmur` (Env1/Velocity -> FilterCutoff),
`content/presets/soft-pad.murmur` (Lfo1 -> FilterCutoff),
`content/presets/wide-saw.murmur` (Velocity -> FilterCutoff), and
`content/presets/wt-morph.murmur` (Lfo1 -> OperatorWavetablePosition) all ship real routes as
working examples.

**Not yet implemented:** meta-modulation (modulating a route's own depth) is
PLANNED. `AlgorithmMorph` is deliberately not a mod destination yet, since
algorithm morph itself isn't implemented (Phase 9).

## Macros

**IMPLEMENTED.** `Patch::macros[8]` (id/name/description/value) round-trips through
save/load and is threaded into every voice as `ModSourceValues::macros` (via
`Engine::loadPatch()`/`noteOn()`), so `ModSource::Macro1`..`Macro8` are live and
routable today -- `docs/COMPETITIVE_ANALYSIS.md` confirms this already matches
Phase Plant's "8 macros, each routable to multiple destinations" model structurally.
Host/plugin-side automation of macro values is wired end to end
(`juce::AudioProcessorValueTreeState`, `Engine::setMacroValue()` -- see
`docs/PLUGIN_ARCHITECTURE.md` "Automation"); a real PLAY/DESIGN/LAB UI to draw/edit
routes visually is still PLANNED (Phase 17). Every factory preset's macros
currently ship with `value: 0.0`, since nothing yet sets them to anything else.

## Why this shipped when it did

The master spec's phased roadmap sequences "Modulation" (mod matrix, LFO, macros) as
Phase 5, after the algorithm graph (Phase 3-4) and before filters (Phase 6). A
single amp envelope + single LFO landed together in an earlier pass of this same
session specifically because Filter 1 needed a modulation source worth
demonstrating. The full 8-envelope/8-LFO/LAYER+GLOBAL-scope target landed in a
later "GATE 5" pass (docs/ROADMAP.md), once the 1-of-each VOICE-scoped version had
already proven the data model, mod matrix execution shape, and Engine/Voice
integration pattern end to end -- expanding a proven single instance to 8 parallel
ones (plus one new scope-selection concept for LFOs) was a much smaller, lower-risk
increment than building all of it at once would have been.
