# Modulation

## Envelope

**IMPLEMENTED.** `envelope::DahdsrEnvelope` (`pw8/envelope/DahdsrEnvelope.hpp`):
Delay/Attack/Hold/Decay/Sustain/Release, per-stage curve shape (0 = linear, higher =
more exponential-feeling), legato retrigger-from-current-level option. One instance
per voice today, driving amplitude directly (`Voice::ampEnvelope`) *and* usable as a
mod matrix source (`ModSource::AmpEnvelope`) for other destinations (e.g. a classic
"envelope opens the filter" route). See `tests/dsp/DahdsrEnvelopeTests.cpp`.

**Not yet implemented:** the master target of 8 independent envelopes per patch,
envelope loop mode, tempo sync. Only the single amplitude envelope exists.

## LFO

**IMPLEMENTED (per-voice).** `lfo::Lfo` (`pw8/lfo/Lfo.hpp`): sine, triangle, saw,
square, sample-and-hold, and smooth-random waveforms; free, retrigger, one-shot, and
tempo-sync modes (a fixed table of musical divisions from 4 bars down to a dotted
1/4, `lfo::kSyncDivisionQuarterNotes`). One instance (`lfo1`) per voice, seeded
deterministically per note for reproducible sample-and-hold/smooth-random sequences.
Tempo sync reads the render/host BPM (`render::Engine::setTempo()`, threaded from
`RenderOptions::bpm` in the native renderer and from the JUCE host playhead in the
plugin). See `tests/dsp/LfoTests.cpp` and
`tests/regression/RenderSanityTests.cpp`'s BPM-tracking end-to-end test.

**Not yet implemented:** the master target of 8 LFOs per patch. A shared **global**
LFO mode (one instance, phase-locked across all voices, rather than one independent
instance per voice) is also PLANNED -- today every voice's LFO1 free-runs
independently from its own note-on, which is the right behavior for `Retrigger`/
`OneShot` modes but means `Free`-mode LFOs across simultaneously-held notes are not
phase-locked to each other.

## Mod Matrix

**IMPLEMENTED (VOICE scope).** `modulation::ModMatrixExecutor` (`pw8/modulation/ModMatrixExecutor.hpp`),
executing `LayerPatch::modRoutes` (a `core::FixedVector<ModRoute, core::kMaxModRoutes>`,
capacity 64) once per voice per sample. No allocation; routes with `source == None`
or `destination == None` are skipped cheaply.

Sources: `Lfo1`, `AmpEnvelope`, `Velocity`, `ChannelPressure`, `PolyAftertouch`,
`MpeSlide`, `Macro1`..`Macro8`.

Destinations: `FilterCutoff` (exponential/semitone-scaled), `FilterResonance`
(additive), `OperatorLevel` (multiplicative, per-node via `targetIndex`), `Pan`
(additive), `OperatorWavetablePosition` (additive, per-node via `targetIndex`,
result clamped 0..1 -- only meaningful for operators on the Wavetable engine).

`tests/unit/ModMatrixTests.cpp` covers: neutral output with no routes, correct
Velocity->FilterCutoff scaling, multiplicative composition of multiple routes to the
same operator, inactive-route skipping, and macro-index resolution.
`content/presets/dark-bass.pw8` (AmpEnvelope/Velocity -> FilterCutoff),
`content/presets/soft-pad.pw8` (Lfo1 -> FilterCutoff),
`content/presets/wide-saw.pw8` (Velocity -> FilterCutoff), and
`content/presets/wt-morph.pw8` (Lfo1 -> OperatorWavetablePosition) all ship real routes as
working examples.

**Not yet implemented:** LAYER and GLOBAL scope (`ModScope` is recorded on every
route, but only VOICE-scoped execution happens -- see `ModMatrixExecutor.hpp`'s
header comment). Meta-modulation (modulating a route's own depth) is PLANNED.
`AlgorithmMorph` is deliberately not a mod destination yet, since algorithm morph
itself isn't implemented (Phase 9).

## Macros

**PARTIAL.** `Patch::macros[8]` (id/name/description/value) round-trips through
save/load and is threaded into every voice as `ModSourceValues::macros` (via
`Engine::loadPatch()`/`noteOn()`), so `ModSource::Macro1`..`Macro8` are live and
routable today -- `docs/COMPETITIVE_ANALYSIS.md` confirms this already matches
Phase Plant's "8 macros, each routable to multiple destinations" model structurally.
What's missing is host/plugin-side automation of macro *values* themselves (there's
no `juce::AudioProcessorValueTreeState` binding yet -- PLANNED, Phase 16) and any UI
to draw/edit routes (Phase 17). Every factory preset's macros currently ship with
`value: 0.0`, since nothing yet sets them to anything else.

## Why this shipped when it did

The master spec's phased roadmap sequences "Modulation" (mod matrix, LFO, macros) as
Phase 5, after the algorithm graph (Phase 3-4) and before filters (Phase 6). Both
landed together in a later pass of this same session specifically because Filter 1
needed a modulation source worth demonstrating (LFO/envelope/velocity -> cutoff is
the single most common subtractive-synth patch-design move), and the mod matrix
needed a second destination beyond "operator level" to be worth building generally
rather than as a cutoff-only special case. Building them together also meant the
`ModDestination` enum could be designed against two real, different consumers
(a per-voice DSP object and a per-node array) instead of one.
