# Modulation

## Envelope

**IMPLEMENTED.** `envelope::DahdsrEnvelope` (`pw8/envelope/DahdsrEnvelope.hpp`):
Delay/Attack/Hold/Decay/Sustain/Release, per-stage curve shape (0 = linear, higher =
more exponential-feeling), legato retrigger-from-current-level option. One instance
per voice today, driving amplitude only (`Voice::ampEnvelope`). See
`tests/dsp/DahdsrEnvelopeTests.cpp` for stage-timing and sustain/release-to-zero
verification.

**Not yet implemented:** the master target of 8 envelopes per patch, envelope loop
mode, tempo sync. Only the single amplitude envelope exists; using an envelope to
modulate anything other than amplitude (filter cutoff, algorithm morph, etc.)
requires the mod matrix below, which doesn't exist yet.

## LFO

**PLANNED** (Phase 5). `pw8/lfo/` is an empty, documented placeholder.

## Mod Matrix / Macros / MSEG / Meta-Modulation

**PLANNED** (Phase 5). `pw8/modulation/` is an empty, documented placeholder for
`ModulationRoute` and the fixed-capacity 64-route matrix. `Patch::macros[8]` exists
in the schema (id/name/description/value) and round-trips through save/load, but
has no routing target yet -- every factory preset's macros are present with
`value: 0.0` and a "Reserved -- not yet routed" description, which is the honest
current state rather than a promise of behavior that doesn't exist.

## What IS live today: per-note (MPE-shaped) expression

**PARTIAL.** `voice::NoteExpression` (`pw8/voice/Voice.hpp`) captures pitch bend,
channel pressure, poly aftertouch, and MPE slide/pitch per voice from MIDI --
architected from day one rather than bolted on later, per the master spec. Of
these, **only pitch bend currently audibly affects sound** (it's summed directly
into the voice's effective frequency in `Voice::renderSample()`). Channel
pressure/poly aftertouch/MPE slide are captured and stored
(`render::Engine::channelPressure()` / `polyAftertouch()` / the CC74 handler in
`controlChange()`) but have no destination to modulate yet -- they'll become mod
matrix sources once that lands.

## Why envelope/expression shipped before the full mod matrix

The master spec's phased roadmap explicitly sequences "Modulation" (mod matrix,
LFO, macros) as Phase 5, after the algorithm graph (Phase 3-4) and before filters
(Phase 6). This pass targets Phase 0/1 solidly plus Phase 2/3 proof-of-architecture,
so the envelope (needed for *any* patch to have a natural note shape) and MPE
capture (needed so the voice/expression data model doesn't need a breaking change
later) are implemented now, while the general-purpose routing matrix that will
eventually connect *any* source to *any* destination is correctly deferred.
