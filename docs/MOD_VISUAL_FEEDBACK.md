# Modulation Visual Feedback

**Status:** Ring clarity pass shipped v1.1.3 (MVP v1.1.2)  
**Related:** [`MODULATION.md`](MODULATION.md), [`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md)

---

## Question

> If a macro or modulation is assigned to a knob, should the knob animate to show live modulation?

**Answer:** Yes — industry standard (Serum/Vital orange mod ring, Hydrasynth mod indicators). MURMUR already had **static** mod-route rings on opt-in knobs; v1.1.2 adds **live ghost pointers** for modulated APVTS params. v1.1.3 thickens rings and separates value arc (inner) from mod route (outer).

---

## Ring legend

Each GlowKnob / decked rotary may show up to three distinct layers. Read **inside → out**:

| Layer | What it means | Where drawn | Visual (v1.1.3) |
|-------|---------------|-------------|-----------------|
| **Value arc** | Current APVTS parameter value (base knob position) | Inner — `ObsidianLookAndFeel` / `DeckedKnobDraw::drawValueArc` on middle deck | **Thick purple/lavender glow arc** (accent colour; warm on macro KOINS). Pointer line aligns to this arc. |
| **Mod route ring** | An active mod-matrix route **to** this parameter | Outer — `GlowKnob::paintOverChildren` via `KnobRingDraw` | **Thick source-coloured arc** outside value arc with visible gap. Arc length = normalized route **depth** (amount), not live mod value. Colours: MW=cyan, Macro=warm, LFO=green, etc. (`palette::modSourceColour`). |
| **Live mod ghost** | Real-time modulated effective value (base + preview offset) | On outer mod ring radius | **Bright glowing dot** on the rim at the modulated position. Moves with MW / macro / LFO preview (12 Hz). |
| **Macro activity ring** | Macro KOIN has ≥1 active **outbound** route | Outer warm ring on macro knobs only | **Warm amber arc** (arc length = macro value 0–1). Featured KOINS get thicker stroke + dim full-track halo. Knob position still shows base macro APVTS value. |

### Visual hierarchy (v1.1.3)

```
        ╭── mod route ring (outer, source colour, thick)
       ╭┴─ value arc (inner, accent/lavender, thick)
      │  ◉  live mod ghost (dot on outer ring)
      ╰── pointer (aligns with value arc)
```

- Mod rings sit **outside** the decked outer rim with a ~4–6 px gap so they never compete with the value arc.
- Drag-hover for mod assignment shows a **violet ellipse** on the outer mod radius.
- **Featured performance KOINS:** value arc uses warm accent; macro activity ring is ~22% thicker with a warm track halo.

### Panel legend (BASIC PLAY)

First KOINS row shows caption: `● value arc   ○ mod route   · live mod` (`PatchFocusPanel::ringLegendLabel_`).

---

## Current UI audit

| Component | Static mod ring | Live mod animation |
|-----------|-----------------|-------------------|
| **GlowKnob** | Yes — outer source-coloured arc = route depth (**thickened v1.1.3**) | Ghost dot on outer ring (**v1.1.2**, brighter v1.1.3) |
| **ConcentricGlowKnob** | Yes — inner/outer mod targets (Filter, OSC) (**thickened v1.1.3**) | Not yet — static depth arc only |
| **PatchFocusPanel / MacroStrip** | Macro knobs: warm activity ring when routed (**featured treatment v1.1.3**) | Activity arc follows macro value |
| **ModMatrixExecutor** | N/A | Engine runs per-sample; UI previews at 12 Hz via `buildModPreviewSources()` |

### What the static ring meant before v1.1.3

- Colour = mod source (macro, LFO, MW, etc.)
- Arc length = normalized route **depth** (amount), not live value
- Only knobs calling `enableModulationTarget()` (Filter, OSC level/WT, etc.)
- **Problem:** thin ~2 px stroke on same radius as value arc — easy to misread as “double value ring”

---

## v1.1.2 MVP behaviour (unchanged semantics)

### Standard APVTS knobs (PatchFocusPanel standard controls)

When `findModDestinationForApvtsParam()` maps a knob to a mod-matrix destination **and** an active route exists:

1. **Static ring** — source colour + depth (outer placement since v1.1.3)
2. **Ghost pointer** — glowing dot on outer ring at **modulated effective value**

Preview sources: macros, mod wheel, expression, sidechain, **layer LFO tick** (message-thread LFO preview at host BPM). Envelope routes do not animate in preview yet.

### Macro KOIN knobs

When a macro has ≥1 active outbound route:

- **Warm activity ring** — arc length tracks macro value (0–1)
- Knob position still shows base macro value from APVTS

---

## Implementation files

| File | Role |
|------|------|
| [`plugin/src/ui/theme/KnobRingDraw.h`](../plugin/src/ui/theme/KnobRingDraw.h) | Shared outer mod-ring layout, thick glow strokes, ghost dot (**v1.1.3**) |
| [`plugin/src/ui/theme/DeckedKnobDraw.h`](../plugin/src/ui/theme/DeckedKnobDraw.h) | Thicker inner value arcs on decked knobs |
| [`plugin/src/ui/theme/ObsidianLookAndFeel.cpp`](../plugin/src/ui/theme/ObsidianLookAndFeel.cpp) | Thicker value arcs on standard rotaries |
| [`plugin/src/ui/ModPreview.hpp`](../plugin/src/ui/ModPreview.hpp) | Offset → modulated param value → normalized 0..1 |
| [`plugin/src/processor/MurmurProcessor.cpp`](../plugin/src/processor/MurmurProcessor.cpp) | `buildModPreviewSources()`, `getHostBpm()` |
| [`plugin/src/ui/components/GlowKnob.cpp`](../plugin/src/ui/components/GlowKnob.cpp) | Mod ring + ghost pointer overlays |
| [`plugin/src/ui/components/PatchFocusPanel.cpp`](../plugin/src/ui/components/PatchFocusPanel.cpp) | Ring legend label, featured KOIN wiring |
| [`plugin/src/ui/components/ModRoutingUi.cpp`](../plugin/src/ui/components/ModRoutingUi.cpp) | `findModDestinationForApvtsParam()` |

---

## Follow-ups (not in MVP)

| Item | Notes |
|------|-------|
| **ConcentricGlowKnob ghost pointers** | Filter cutoff/reso outer+inner live mod |
| **Envelope-driven preview** | Show amp/env routes during held notes |
| **Master-bus / Quasar destinations** | `applyMasterBus()` preview for GLOBAL knobs |
| **Mod matrix row pulse** | Highlight rows when source value changes |
| **Reduce 12 Hz poll** | Push notification when routes or macro values change |
| **Multi-route stacking** | Multiple outer rings when several sources hit one param |

---

## Logic verify

1. Load **Interstellar/Spatial/001-nebula-drift** — feature macro KOINS show **thick warm outer rings** when routed; inner value arc stays lavender/warm.
2. **BASIC PLAY** — confirm caption `● value arc   ○ mod route   · live mod` under intro text.
3. **Advanced → OSC** — assign MW → Op0 Level; move mod wheel: **bright ghost dot** moves on outer ring; inner pointer stays at base until you move the knob.
4. **Basic PLAY** — sweep SPACE macro: outer warm ring arc length follows macro; routed targets audibly move.
5. **MOD tab** — add LFO1 → Filter Cutoff (Layer scope); ghost dot should oscillate on Filter panel cutoff knob (outer ring, thicker stroke).
