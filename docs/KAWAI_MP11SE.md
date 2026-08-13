# Kawai MP11SE + MURMUR (Ben)

Out-of-the-box performance mapping for Ben’s **Kawai MP11SE** master keyboard in Logic Pro with MURMUR.

The MP11SE sends **mod wheel** and **pitch bend** over USB by default. Its **four panel knobs (A–D)** and **expression pedal** must be assigned to MIDI CC numbers in a **MIDI OUT zone** before Logic sees them. MURMUR listens for those CCs on every patch — no MIDI learn required.

## One-time MP11SE setup

1. Connect MP11SE to the Mac via **USB** (or MIDI if you prefer — same CC layout).
2. Press **INT/MIDI** until the zone list appears.
3. Select or create a **MIDI OUT** zone (e.g. “MURMUR” or “Logic”).
4. Set **Transmit Channel** to **1** (or match Logic’s track channel).
5. **Knob Assign** (zone EDIT → Knob Assign):

   | Knob | Send CC | GM name | MURMUR role |
   |------|---------|---------|-------------|
   | A | **74** | Brightness | Macro 3 + mod-matrix brightness |
   | B | **71** | Harmonic | Macro 4 |
   | C | **73** | Attack | Macro 5 |
   | D | **91** | Reverb send | Macro 6 (SPACE / FX depth on many patches) |

6. **Expression pedal:** Zone or global pedal setup → assign to **CC11** (Expression). Default on many MP11SE setups; confirm in pedal assign menu.
7. **Sustain:** Right pedal → **CC64** (standard; works in Logic + MURMUR).
8. Optional: map **channel volume slider** to **CC7** if your zone allows it → MURMUR **master gain**.

Save the zone as a **Setup** on the MP11SE so this layout loads whenever you play MURMUR.

> **Note:** MP11SE **volume sliders do not send MIDI** unless routed through a zone/controller assignment. Use CC7 from a slider only if you’ve explicitly programmed it; otherwise use Logic’s fader or MURMUR’s master in the UI.

## Logic Pro

1. Create an **Instrument** track → **AU Instruments → Murmur → MURMUR**.
2. Set track **MIDI channel** to **1** (match the MP11SE zone).
3. Input: **MP11SE** (USB MIDI port).
4. Load any **factory** preset — all 800 include mod wheel, expression, velocity, and category-tuned macro routes.
5. **Smart Controls:** See [`LOGIC_SMART_CONTROLS.md`](LOGIC_SMART_CONTROLS.md) for a ready-made 8-knob layout (macros + mod wheel + expression readouts).

## What moves when you play

| Your control | MIDI | MURMUR |
|--------------|------|--------|
| Mod wheel | CC1 | Filter cutoff (every patch; PLAY badge shows depth) |
| Expression pedal | CC11 | Filter resonance / timbre (mod matrix) **and** Macro 2 |
| Knob A | CC74 | Macro 3 + brightness → filter (mod matrix) |
| Knob B | CC71 | Macro 4 |
| Knob C | CC73 | Macro 5 |
| Knob D | CC91 | Macro 6 |
| Key pressure | Aftertouch | Filter / level (pads & ambient; many bass/lead patches) |
| Velocity | — | Filter or operator level (category-dependent) |
| Sustain pedal | CC64 | Hold |
| Pitch bend | PB | ±2 semitones |

Macros map to patch-specific sound design (PUNCH, WARMTH, SPACE, etc.). The **Knobs of Interest** panel on the PLAY screen shows the six most important controls for the loaded preset.

## Custom routing

Open **Mod Matrix (M)** in MURMUR:

- **MW** — mod wheel (CC1)
- **EXP** — expression (CC11)
- Drag chips onto ringed knobs or use Cutoff / Resonance quick targets.

Factory presets are regenerated with `ensure_standard_midi_layout()`; see `docs/MIDI_CONTROLLERS.md` for the full matrix spec.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Mod wheel works, knobs don’t | Knobs not assigned to CC in MIDI OUT zone — redo step 5 above. |
| Expression dead | Confirm pedal sends **CC11**; check Logic MIDI monitor. |
| Wrong channel | Match MP11SE zone channel and Logic track channel. |
| Notes hang on stop | Fixed in recent builds — update MURMUR AU; transport stop sends all-sound-off. |
| Presets missing | Run installer or `rsync -a content/presets/factory/ "$HOME/Library/Application Support/MURMUR/Presets/factory/"` |

## Related docs

- [`MIDI_CONTROLLERS.md`](MIDI_CONTROLLERS.md) — full CC / mod-source reference
- [`INSTALL.md`](INSTALL.md) — Mac installer for Ben
