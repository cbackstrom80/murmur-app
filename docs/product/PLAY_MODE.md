# MURMUR — PLAY Mode

PLAY mode is MURMUR's **performance surface** — the screen you use while writing and playing, not while programming oscillators from scratch.

The **OBSIDIAN** skin uses a dark, minimal layout: cool cyan for structure (graph, filter, FX), warm amber for the macros and Knobs of Interest.

---

## Layout at a glance

```
┌─────────────────────────────────────────────────────────────┐
│  MURMUR    [◀ preset name ▶]   BROWSE  LOAD  SAVE          │  ← Patch bar
├─────────────────────────────────────────────────────────────┤
│                                                             │
│              Algorithm graph (E0–E7 + GLOBAL)               │  ← Tap a node to edit
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  Knobs of Interest (6)     │  Mod wheel / Expression badges │
│  [Macro knobs + Cutoff]    │  Mod Matrix (M) entry          │
├─────────────────────────────────────────────────────────────┤
│  Macros 1–8 strip (amber)                                   │
└─────────────────────────────────────────────────────────────┘
```

Use the **view toggle** to switch between compact PLAY and advanced pages (OSC, FILTER, ENV, MOD, FX).

---

## Patch bar

| Control | Action |
|---------|--------|
| **◀ ▶** | Previous / next preset in the current browse filter |
| **BROWSE** | Open searchable preset overlay (category, mood, tags) |
| **LOAD...** | Open `.pw8` file from disk |
| **SAVE** | Write current patch to `.pw8` |
| Preset name | Shows loaded patch metadata |

**Favorites:** star presets in the browse overlay — stored in `~/Library/Application Support/MURMUR/favorites.json`.

---

## Algorithm graph

The graph shows **eight operator nodes (E0–E7)** and their modulation edges — the "algorithm" of the current patch.

- **Tap a node** to open the operator editor (engine type, level, ratio, wavetable, etc.).
- **GLOBAL** scope selects layer-wide controls (filter, layer gain, master FX).
- Live signal pulses travel along active edges when audio is playing.

This is MURMUR's signature visual — you see *how* a sound is wired, not just a list of parameters.

---

## Knobs of Interest (KOINS)

Six knobs curated **per preset** for hands-on performance:

- Authored in each `.pw8` file (`uiFocus` block), or **inferred** from macro routes and filter/mod wiring.
- Typical knobs: **Cutoff**, **Reso**, patch macros (PUNCH, WARMTH, SPACE, …).
- On the **PLAY** screen, badges show live **Mod Wheel (CC1)** and **Expression (CC11)** routing.

**Policy:** every factory preset exposes at least **4** focus knobs; target is **6**.

---

## Macros 1–8

Eight host-automatable performance controls:

- Mapped to Logic **Smart Controls** and MIDI CC (see [Performance](PERFORMANCE.md)).
- Each patch names its macros (e.g. PUNCH, BLOOM, SHIMMER) — the AU parameter list always shows Macro 1–8.
- At least one macro is wired into the mod matrix on every factory preset.

---

## Mod Matrix (M)

Open from the Knobs of Interest panel or advanced MOD page:

1. **Source chips** — LFO, ENV, Velocity, MW, EXP, Macros…
2. **Drag** a chip onto a ringed knob, or arm a source → click **Cutoff** / **Resonance**.
3. **Connections list** — edit amounts, remove routes.

Factory presets ship with standard MIDI routes (mod wheel → cutoff, expression → resonance, velocity, aftertouch on pads).

---

## Scope: E0–E7 vs GLOBAL

| Scope | What you edit |
|-------|----------------|
| **E0–E7** | Individual operator (engine, level, pan, per-op filter) |
| **GLOBAL** | Layer filter, layer gain/pan, insert FX, arpeggiator |

The scope selector sits near the graph — switching scope changes which panel appears below.

---

## Advanced pages (brief)

| Page | Purpose |
|------|---------|
| **OSC** | Operator parameters for selected node |
| **FILTER** | Global Filter 1 + key tracking |
| **ENV** | Amp and mod envelopes (8 slots) |
| **MOD** | Full mod matrix + LFO rates |
| **FX** | Insert chain + master reverb/EQ/comp |

---

## Tips

- **Start in PLAY** — load a factory preset, turn KOINS and macros before diving into OSC.
- **Mod wheel first** — every factory patch responds; use it as your primary filter performance control.
- **Browse by category** — Pads for atmosphere, Sequences for motion, Basses for weight.
- **Save variants** — tweak macros, SAVE AS a new `.pw8` for your personal library.

→ [Presets guide](PRESETS.md) · [Sound design primer](SOUND_DESIGN.md)
