# Arpeggiator

**IMPLEMENTED.** `sequencer::Arpeggiator` (`pw8/sequencer/Arpeggiator.hpp`,
`pw8/sequencer/ArpeggiatorTypes.hpp`). Closes Phase 12's arpeggiator half (step
sequencer/MSEG remain PLANNED).

## Design

Performance-wide (`Patch::arpeggiator`, not per-layer -- MIDI dispatch is
engine-wide today regardless of layer). When `enabled`, `render::Engine::noteOn/
noteOff` redirect incoming MIDI to `Arpeggiator::noteHeld()/noteReleased()` instead
of triggering voices directly; `Engine::process()` ticks the arpeggiator once per
sample and translates whatever it emits into the *same* `triggerNoteOnDirect()/
triggerNoteOffDirect()` calls a real performer's MIDI would produce -- the
arpeggiator is indistinguishable from a performer as far as voice allocation,
envelopes, and the mod matrix are concerned.

Two independent, simultaneously-cycling counters give the classic "polymetric" feel:

- The **step pattern** (up to 64 steps, `ArpStep`: enabled/rest, octave offset, gate,
  probability, ratchet count, tie, velocity scale, accent).
- The **note sequence** (the held chord expanded across `octaveRange`, ordered per
  `ArpMode`: Up/Down/UpDown/DownUp/AsPlayed/Random/Chord).

Both advance one position per firing step but wrap at their own length, so a
5-step pattern over a 3-note chord doesn't repeat the same (step, note) pairing
until step 15 -- proven directly in `tests/unit/ArpeggiatorTests.cpp`'s polymetric
test, which hand-traces the exact expected sequence for that combination.

## Two design bugs caught before they shipped

Worth recording because both were caught by *reasoning through the sample-accurate
timeline before writing the test*, not by the test failing first:

1. **Tie's naive "extend the pending note-off" didn't work.** A note's scheduled
   note-off (at `gate * stepDuration`) almost always fires *before* the next tick
   even reaches a tied step, since `gate <= 1.0`. By the time the tie step arrived,
   there was nothing left to extend. Fixed with lookahead: when a step fires, it
   scans forward through any immediately-following `tie` steps and adds their full
   duration to its own note-off delay *up front* -- the tied step itself does
   nothing when it's actually reached, because it was already accounted for.
2. **Ratchet sub-hits were firing simultaneously instead of spread across the
   step**, and advancing through *different notes* per sub-hit instead of repeating
   the *same* note. Fixed with a second delayed-trigger scheduler
   (`PendingTrigger`, alongside the existing `PendingNoteOff`): the first ratchet
   sub-hit fires immediately, the rest are scheduled at `subStepSamples * r` and
   fire on their own later sample; the note sequence now advances exactly once per
   *step*, not once per ratchet sub-hit.

Both fixes are covered by dedicated tests (`[tie]`, `[ratchet]`) plus the
render-level regression test below.

## Testing

`tests/unit/ArpeggiatorTests.cpp` (10 cases): per-mode note sequence generation
(including hand-verified exact sequences for Up/Down/UpDown/AsPlayed), Chord mode
firing every held note together, latch keeping the pattern alive after release vs.
stopping without it, seeded-probability determinism, ratchet producing the right
*count* of sub-hits, tie suppressing a step's retrigger, and the polymetric
step/note-sequence independence.

`tests/regression/RenderSanityTests.cpp`'s arpeggiator case renders one held
3-note chord for 2 seconds through the full `Engine`, twice (arp off, arp on at
8 Hz), and counts amplitude "onsets" (windowed-RMS rising edges) in the actual
audio: **1 onset without the arp, 16 with it** -- exactly matching 8 Hz x 2s, proven
in rendered audio rather than asserted against internal state.

## Content

`content/presets/arp-pluck.pw8`: bright saw pluck, Filter 1 shaping the tone,
Up mode tempo-synced to 1/16 notes across 2 octaves, latch on, an 8-step pattern
with a real accent, a ratcheted double-hit, and a deliberate rest -- not a plain
uniform up-arp.

## PLAY UI (P0)

**IMPLEMENTED** in PLAY mode (Basic + Advanced):

- **Arp launcher chip** (`ArpLauncherChip`) — beside the Basic/Advanced toggle: ARP
  on/off (`arpEnabled`), live rate readout (sync division or Hz), cyan pulse when
  active. Click opens the drawer; keyboard **`A`** also opens it.
- **Arp drawer** (`ArpPanelOverlay`) — right-side panel (`layout::kArpDrawerWidth` =
  420px): scalar GlowKnobs for mode, rate mode, rate Hz, sync division, octave
  range, step count, latch; read-only **step strip** (`ArpStepStrip`) shows
  rest/tie/ratchet/accent/probability glyphs from the loaded patch. Esc or
  click-outside dismisses.

Step editing, playhead position, and per-step APVTS remain **PLANNED** (P1+).

## What's PLANNED, not implemented

- **Global/LFO-style free-running arps unsynced to note-on** -- today the pattern
  position resets implicitly whenever `patternNotes_` changes (a fresh chord starts
  the pattern from wherever `currentStepIndex_`/`noteSequenceIndex_` already were,
  not from step 0 -- there's no explicit "retrigger the pattern on new chord" mode
  yet, only the continuous-running behavior).
- **Per-step destination beyond note generation** -- the master spec's "mod
  sequencer" (separate modulation lanes targeting filter cutoff, wavetable
  position, pan, macros) is architecturally adjacent (it would reuse `ArpStep`'s
  shape and the same tick-driven clock) but not implemented; today's steps only
  ever produce note events.
- **MIDI channel handling for arp output** -- arpeggiated notes are emitted on
  channel 0 regardless of which channel(s) the held notes came in on (Chord mode is
  the exception -- it preserves each held note's original channel).
