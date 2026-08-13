# Logic Pro Smart Controls + MURMUR (Ben)

Template for mapping **MURMUR** parameters to Logic **Smart Controls** on an instrument track, paired with the [Kawai MP11SE hardware layout](KAWAI_MP11SE.md).

Hardware knobs already drive MURMUR macros via MIDI CC (no Smart Control required for live play). Smart Controls add **on-screen feedback**, **trackpad/touch mixing**, and **automation lanes** for the same parameters.

## Prerequisites

1. Logic Pro 10.7+ (Smart Controls layout editor).
2. Instrument track with **AU Instruments → Murmur → MURMUR**.
3. MP11SE zone programmed per [`KAWAI_MP11SE.md`](KAWAI_MP11SE.md) (optional but recommended).

## Recommended Smart Control layout (8 knobs)

Use **Layout → Edit…** on the Smart Controls pane, then assign each control:

| Smart Control | MURMUR parameter (AU) | MP11SE hardware | Role |
|---------------|----------------------|-----------------|------|
| Knob 1 | **Macro 1** | — | Primary patch macro (PUNCH, EDGE, WARMTH, RATE, SPACE…) |
| Knob 2 | **Macro 2** | Expression (CC11) | Timbral macro; pedal also drives mod-matrix Expression |
| Knob 3 | **Macro 3** | Knob A (CC74) | Brightness / timbre |
| Knob 4 | **Macro 4** | Knob B (CC71) | Harmonic content |
| Knob 5 | **Macro 5** | Knob C (CC73) | Attack / envelope shape |
| Knob 6 | **Macro 6** | Knob D (CC91) | Space / FX depth |
| Knob 7 | **Mod Wheel (CC1)** | Mod wheel | Filter cutoff performance (read-only from MIDI; badge on PLAY screen) |
| Knob 8 | **Expression (CC11)** | Expression pedal | Resonance/level route (read-only from MIDI; badge on PLAY screen) |

### How to assign in Logic

1. Open the **Smart Controls** pane (B).
2. Click **Layout** menu → **Edit…** (or right-click a knob → **Assign…**).
3. In the parameter list, expand **MURMUR** (or **Murmur** under the plug-in name).
4. Choose the parameter from the table above.
5. Repeat for each knob; **Save as Default** or **Save as…** → `MURMUR Performance.patch` in `~/Music/Audio Music Apps/Patch Settings/Smart Controls/`.

Macro names in the plug-in UI match patch metadata (e.g. PUNCH, SPACE). The AU parameter list always shows **Macro 1** … **Macro 8** regardless of label.

## Alternate: mix + filter on screen

If Ben prefers channel-style mixing on Smart Controls:

| Control | MURMUR parameter |
|---------|------------------|
| Large knob | **Master Gain** (CC7 if slider sends it from MP11SE) |
| Knob 2 | **Filter Cutoff** (`filter…` global cutoff param) |
| Knob 3 | **Filter Resonance** |
| Knobs 4–6 | **Macro 1–3** |

Filter params are patch-scoped; macros remain the most portable choice across all 800 factory presets.

## PLAY screen badges (plug-in UI)

On MURMUR’s **PLAY** view, **Knobs of Interest** shows live status lines:

- **Mod Wheel (CC1)** — position % and mod-matrix destination (e.g. GLOBAL FILTER CUTOFF).
- **Expression (CC11)** — position % and destination (e.g. FILTER RESONANCE).

These mirror what Smart Controls knobs 7–8 display when mapped to **Mod Wheel (CC1)** and **Expression (CC11)**.

## Automation

- **Macros 1–8** — full automation read/write in Logic.
- **Mod Wheel / Expression** — MIDI-driven; APVTS params update for display. Automate in Logic via MIDI draw on the region or Smart Control lanes (may fight live pedal input on the same CC).
- Prefer automating **macros** for mix moves; keep **mod wheel / expression** for performance.

## Troubleshooting

| Issue | Fix |
|-------|-----|
| MURMUR not in parameter list | Rescan AU plug-ins; confirm track uses MURMUR AU (not VST3 in Logic). |
| Knob moves plug-in but not Smart Control | Map Smart Control to the same **Macro N** parameter; hardware already writes macros via CC. |
| Double-fighting on CC11 | Expression updates Macro 2 **and** mod-matrix Expression — intentional; avoid automating Macro 2 while performing on the pedal. |
| Layout lost on new track | Save Smart Control patch; set as default for software instruments or duplicate the configured track. |

## Related

- [`KAWAI_MP11SE.md`](KAWAI_MP11SE.md) — keyboard zone + CC programming
- [`MIDI_CONTROLLERS.md`](MIDI_CONTROLLERS.md) — full CC / mod-source reference
