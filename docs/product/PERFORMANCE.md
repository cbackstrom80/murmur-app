# MURMUR — Performance & MIDI

MURMUR is built for **hands-on playing** — mod wheel, expression, aftertouch, and hardware knobs map to sound without MIDI learn setup on factory patches.

---

## Standard factory MIDI layout

Every factory preset includes these routes minimum:

| Control | MIDI | MURMUR |
|---------|------|--------|
| **Mod wheel** | CC1 | Filter cutoff (+18 to +28 st depending on category) |
| **Expression** | CC11 | Resonance or level + Macro 2 |
| **Velocity** | Note on | Filter cutoff or operator level |
| **Aftertouch** | Channel pressure | Filter (pads & ambient) |
| **Brightness** | CC74 | Filter cutoff + Macro 3 |

PLAY mode shows live **Mod Wheel** and **Expression** badges with destination and depth.

---

## Performance CC → Macro map

Hardware knobs can drive MURMUR macros directly (before the engine updates):

| CC | GM name | Macro / param |
|----|---------|---------------|
| 7 | Volume | Master gain |
| 11 | Expression | Macro 2 |
| 74 | Brightness | Macro 3 |
| 71 | Harmonic | Macro 4 |
| 73 | Attack | Macro 5 |
| 91 | Reverb send | Macro 6 |
| 93 | Chorus send | Macro 7 |

CC1 (mod wheel) uses the dedicated mod-wheel path — not remapped to a macro.

→ Full spec: [`../MIDI_CONTROLLERS.md`](../MIDI_CONTROLLERS.md)

---

## Kawai MP11SE (Ben's keyboard)

One-time zone programming assigns panel knobs to the CC map above.

| MP11SE control | CC | Result |
|----------------|-----|--------|
| Mod wheel | 1 | Filter brightness |
| Expression pedal | 11 | Resonance + Macro 2 |
| Knob A | 74 | Macro 3 |
| Knob B | 71 | Macro 4 |
| Knob C | 73 | Macro 5 |
| Knob D | 91 | Macro 6 (SPACE) |
| Aftertouch | — | Filter / level |

→ Step-by-step: [`../KAWAI_MP11SE.md`](../KAWAI_MP11SE.md)

---

## Logic Pro Smart Controls

Map on-screen knobs to MURMUR parameters for visual feedback and automation:

| Smart Control | MURMUR parameter |
|---------------|------------------|
| Knobs 1–6 | Macro 1–6 |
| Knob 7 | Mod Wheel (CC1) |
| Knob 8 | Expression (CC11) |

→ Template: [`../LOGIC_SMART_CONTROLS.md`](../LOGIC_SMART_CONTROLS.md)

---

## Logic automation

- **Macros 1–8** — full read/write automation lanes.
- **Mod wheel / Expression** — MIDI-driven; mirrored as AU parameters for display.
- Prefer automating **macros** for mix moves; keep wheel/pedal for live performance.

---

## Sustain & pitch bend

| Control | Behavior |
|---------|----------|
| **Sustain (CC64)** | Standard hold |
| **Pitch bend** | ±2 semitones (channel-wide) |

---

## Custom mod routing

1. Open **Mod Matrix (M)**.
2. Drag **MW**, **EXP**, **LFO**, or **Macro** chips onto ringed knobs.
3. Edit amounts in the connections list.

Routes save with the `.pw8` patch.

---

## MIDI learn

**Not yet implemented.** Use mod matrix drag-and-drop or edit `.pw8` modRoutes for custom setups.

---

## Tips for live play

- **Mod wheel** is your primary filter performance tool on every factory patch.
- **Expression** adds resonance and timbral depth without touching the UI.
- **Knobs of Interest** mirror the most important macros for the loaded preset — check labels (WARMTH, SPACE, PUNCH).
- On **pads**, play legato chords — Ben MVP polyphony fix prevents infinite voice stacking.

→ [Quick Start](QUICK_START.md) · [PLAY Mode](PLAY_MODE.md)
