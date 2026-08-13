# MURMUR — Sound Design

A musician's guide to how MURMUR makes sound — without opening the engine source code.

---

## The big idea: eight operators, one algorithm

MURMUR is built around **eight operator slots** (E0–E7) wired together as an **algorithm graph**. Each operator runs one of **eight engine types**. The graph defines who modulates whom — FM stacks, parallel layers, feedback paths — before the mixed signal hits the **global filter** and **FX chain**.

You don't need to design from init: **800 factory presets** demonstrate every engine and archetype. Use them as templates — LOAD, tweak KOINS, SAVE AS.

---

## Eight engine types

| Engine | Character | Typical role |
|--------|-----------|--------------|
| **Classic** | Sine, saw, square, triangle | Sub, body, simple carriers |
| **Wavetable** | Morphing digital frames | Evolving pads, plucks, digital texture |
| **FM** | Frequency modulation | Bells, bass, metallic, aggressive |
| **Phase** | Phase distortion / bend | Sync-like digital, harsh harmonics |
| **Additive** | Harmonic partials | Organ, glass, controlled spectrum |
| **Noise** | Filtered noise | Breath, air, percussion layer |
| **Granular** | Grain clouds | Ambient spray, texture |
| **Resonator** | Modal / string models | Plucks, mallets, physical tone |

Select an operator in the graph → **OSC** page to change engine and parameters.

---

## Algorithm topologies

MURMUR includes preset algorithm graphs (parallel carriers, serial FM, feedback bell, triple-carrier, etc.):

- **Parallel** — all operators sum to output (ensemble, supersaw-style)
- **Serial FM** — modulators feed carriers in a chain
- **Feedback** — operator feeds back into itself for inharmonic/decay tones
- **Mixed** — combinations for bass, pad, and seq archetypes

The **algorithm graph view** shows the active topology — edges indicate modulation routing.

---

## Filter

**Global Filter 1** — lowpass (and other modes) with cutoff, resonance, key tracking.

- Mod wheel, LFOs, envelopes, and macros commonly target **cutoff** and **resonance**.
- Per-operator filters available on individual engines when the patch requires it.

---

## Envelopes & LFOs

- **8 envelopes** per layer — Env1 is typically the **amp envelope**; Env2–8 are free for filter, pitch, or mod targets.
- **8 LFOs** per layer — sine/triangle/saw/square, free or tempo-synced.
- **Legato** on amp envelope (enabled on factory pads) — retrigger from current level on overlapping notes.

---

## Modulation matrix

Route any **source** to any **destination**:

| Sources | Destinations |
|---------|--------------|
| LFO 1–8, Env 1–8 | Global filter cutoff / resonance |
| Velocity, aftertouch | Operator level, pan |
| Mod wheel, expression | Wavetable position |
| Macros 1–8 | Per-operator filter |

Open **Mod Matrix (M)** in PLAY mode — drag **MW** or **EXP** chips onto ringed knobs.

→ Full reference: [`../MIDI_CONTROLLERS.md`](../MIDI_CONTROLLERS.md)

---

## FX chain

| Slot | Algorithms |
|------|------------|
| **Insert ×3** | Tape delay, chorus, saturation, freq-shift echo, node delay |
| **Master ×4** | Reverb, EQ, compressor, limiter |

Factory pads often ship with **tape delay + long reverb**; basses stay dry or lightly compressed.

Use the **FX** page or wireframe view to reorder inserts (where supported).

---

## Layers & Stack

| Mode | Behavior |
|------|----------|
| **Single A** | One layer (default) |
| **Stack** | Layer B summed — thicker unison stacks, dual timbres |

**Unison** — duplicate voices with detune and stereo spread (common on dream-pop pads).

---

## Arpeggiator

Per-patch arpeggiator — enable on sequence presets for rhythmic patterns. Syncs to host tempo when enabled.

→ Details: [`../ARPEGGIATOR.md`](../ARPEGGIATOR.md) (technical reference)

---

## Patch format (.pw8)

Patches save as **JSON** (`.pw8`) — human-readable, diff-friendly, agent-generatable.

Contains: operators, algorithm, envelopes, LFOs, mod routes, FX, macros, uiFocus, metadata.

→ Schema: [`../PATCH_FORMAT.md`](../PATCH_FORMAT.md) (developer reference)

---

## Workflow suggestions

1. **Load close, tweak macros** — fastest path to a usable sound.
2. **Swap one operator engine** — e.g. Classic → Wavetable on E1 for instant character change.
3. **One mod route** — LFO → cutoff at low depth for movement without overwhelming.
4. **SAVE AS** — build a personal library beside the factory bank.

→ [PLAY Mode](PLAY_MODE.md) · [Presets](PRESETS.md)
