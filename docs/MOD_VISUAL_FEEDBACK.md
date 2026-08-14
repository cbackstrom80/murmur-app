# Modulation Visual Feedback

**Status:** MVP shipped v1.1.2  
**Related:** [`MODULATION.md`](MODULATION.md), [`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md)

---

## Question

> If a macro or modulation is assigned to a knob, should the knob animate to show live modulation?

**Answer:** Yes — industry standard (Serum/Vital orange mod ring, Hydrasynth mod indicators). MURMUR already had **static** mod-route rings on opt-in knobs; v1.1.2 adds **live ghost pointers** for modulated APVTS params.

---

## Current UI audit (pre-ship)

| Component | Static mod ring | Live mod animation |
|-----------|-----------------|-------------------|
| **GlowKnob** | Yes — `enableModulationTarget()` draws source-coloured arc = route depth | **Added v1.1.2** — ghost dot at modulated effective value |
| **ConcentricGlowKnob** | Yes — inner/outer mod targets (Filter, OSC) | Not yet — static depth arc only |
| **PatchFocusPanel / MacroStrip** | Macro knobs had no route indicator | **Added v1.1.2** — warm activity ring when macro has active routes |
| **ModMatrixExecutor** | N/A | Engine runs per-sample; UI now previews at 12 Hz via `buildModPreviewSources()` |

### What the static ring meant before

- Colour = mod source (macro, LFO, MW, etc.)
- Arc length = normalized route **depth** (amount), not live value
- Only knobs calling `enableModulationTarget()` (Filter, OSC level/WT, etc.)

---

## v1.1.2 MVP behaviour

### Standard APVTS knobs (PatchFocusPanel standard controls)

When `findModDestinationForApvtsParam()` maps a knob to a mod-matrix destination **and** an active route exists:

1. **Static ring** — source colour + depth (unchanged)
2. **Ghost pointer** — small dot on the dial rim at the **modulated effective value** (base APVTS + preview mod offset)

Preview sources: macros, mod wheel, expression, sidechain, **layer LFO tick** (message-thread LFO preview at host BPM). Envelope routes do not animate in preview yet.

### Macro KOIN knobs

When a macro has ≥1 active outbound route:

- **Warm activity ring** — arc length tracks macro value (0–1)
- Knob position still shows base macro value from APVTS

### Implementation files

| File | Role |
|------|------|
| [`plugin/src/ui/ModPreview.hpp`](../plugin/src/ui/ModPreview.hpp) | Offset → modulated param value → normalized 0..1 |
| [`plugin/src/processor/PatchworkEightProcessor.cpp`](../plugin/src/processor/PatchworkEightProcessor.cpp) | `buildModPreviewSources()`, `getHostBpm()` |
| [`plugin/src/ui/components/GlowKnob.cpp`](../plugin/src/ui/components/GlowKnob.cpp) | Ghost pointer in `paintOverChildren()` |
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

---

## Logic verify

1. Load **Interstellar/Spatial/001-nebula-drift** — feature macro KOINS show warm rings when routed.
2. **Advanced → OSC** — assign MW → Op0 Level; move mod wheel: ghost dot should move on Level knob.
3. **Basic PLAY** — sweep SPACE macro: ring arc length follows macro; routed targets audibly move.
4. **MOD tab** — add LFO1 → Filter Cutoff (Layer scope); ghost dot should oscillate on Filter panel cutoff knob.
