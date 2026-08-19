# Knob Ring Semantics

**Status:** v1.1.3+ (visual hierarchy pass)  
**Related:** [`MOD_VISUAL_FEEDBACK.md`](MOD_VISUAL_FEEDBACK.md), [`plugin/src/ui/theme/KnobRingDraw.h`](../plugin/src/ui/theme/KnobRingDraw.h)

---

## Overview

MURMUR PLAY-mode knobs can show up to **three concentric visual layers** plus drag-assignment chrome. Read them **inside → out**:

| Layer | Meaning | Drawn by | Visual (v1.1.3+) |
|-------|---------|----------|------------------|
| **Value arc** | Current APVTS parameter value (base knob position) | `ObsidianLookAndFeel` / `DeckedKnobDraw::drawValueArc` on the middle deck | Thick purple/lavender glow arc; warm accent on macro KOINS. **Pointer aligns to this arc** (`ObsidianRotary` convention). |
| **Mod route ring** | Active mod-matrix route **to** this parameter | `GlowKnob::paintOverChildren` via `KnobRingDraw` | Thick **source-coloured** arc **outside** the value arc with a visible gap. Arc length = normalized route **depth** (amount), not live mod value. |
| **Live mod ghost** | Real-time modulated effective value (base + preview offset) | `KnobRingDraw::drawLiveModGhost` on outer mod radius | Bright glowing **dot** on the rim at the modulated position (~12 Hz preview). |
| **Macro activity ring** | Macro KOIN has ≥1 active **outbound** route | Warm ring on macro knobs only | Amber arc length tracks macro value 0–1. Featured KOINS get thicker stroke + dim full-track halo. Knob position still shows base macro APVTS value. |

---

## Visual hierarchy

```
        ╭── mod route ring (outer, source colour, thick)
       ╭┴─ value arc (inner, accent/lavender, thick)
      │  ◉  live mod ghost (dot on outer ring)
      ╰── pointer (aligns with value arc only)
```

- Mod rings sit **outside** the decked outer rim (~5–8 px gap) so they never compete with the value arc.
- **Drag-hover** for mod assignment: violet ellipse on the outer mod radius.
- **Featured performance KOINS:** warm value arc, ~35% thicker macro activity ring, warm track halo when routed.

---

## Source colours (mod route ring)

From `palette::modSourceColour()` — same as MOD tab chips:

| Source | Colour cue |
|--------|------------|
| Mod wheel | Cyan |
| Expression | Teal |
| Macro 1–8 | Warm amber family |
| LFO 1–8 | Green |
| Envelope 1–8 | Orange |
| Sidechain | Accent violet |
| Aftertouch / pressure | Secondary tints |

---

## Where rings appear

| Component | Mod route ring | Live ghost | Notes |
|-----------|----------------|------------|-------|
| **GlowKnob** | Yes (opt-in via `enableModulationTarget`) | Yes | Filter, OSC level, standard PLAY knobs with APVTS↔mod dest mapping |
| **ConcentricGlowKnob** | Yes (inner/outer targets) | Yes | Filter cutoff/reso dual dial |
| **TripleGlowKnob** | Yes (outer/middle/inner targets) | Yes | FM, phase, unison triplets |
| **MacroStrip / PatchFocusPanel** | Activity ring when macro routes out | N/A | Not a mod destination ring |

---

## UI legend

**BASIC PLAY** — first KOINS row shows:

`● value arc   ○ mod route   · live mod`

(`PatchFocusPanel::ringLegendLabel_`)

**Depth popover** — after assigning a mod route (click or drag), a small **DEPTH** bar appears under the knob; drag horizontally to adjust route amount.

---

## Developer notes

| File | Role |
|------|------|
| `KnobRingDraw.h` | Shared outer layout, thick glow strokes, ghost dot |
| `DeckedKnobDraw.h` | Thicker inner value arcs (decked knobs) |
| `ObsidianLookAndFeel.cpp` | Standard rotary value arcs |
| `ModPreview.hpp` | Ghost pointer: mod offset → effective param → 0..1 |
| `GlowKnob.cpp` | Polls mod routes + preview at 8–12 Hz |

**Pointer rule:** Only the inner value arc and standard pointer reflect the raw APVTS value. The ghost dot shows where modulation would land; it does not move the pointer.

---

## Logic verify

1. Load a patch with routed feature macros — warm outer activity rings on KOINS; inner arc shows macro value.
2. BASIC PLAY — confirm ring legend caption under intro text.
3. Assign MW → Filter Cutoff — move mod wheel: ghost dot moves on **outer** ring; pointer stays at base until you move the knob.
4. Right-click a modded knob — route removed, outer ring clears.
