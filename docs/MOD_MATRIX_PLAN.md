# Mod Matrix — Research & Implementation Plan

**Status:** PLAY-mode mod routing is **partially implemented**. This document captures current behavior, gaps, and a phased plan to make the matrix **visually appealing** and **operationally concrete**.

---

## Current State (as of Aug 2026)

### What works

| Capability | Implementation |
|---|---|
| Arm mod source (click chip) | `ModAssignmentController` + `ModSourceChip` |
| Assign to Global/Engine filter Cutoff/Resonance | `ModSourceStrip` destination buttons, `GlowKnob` click |
| Drag source → ringed knob | `ModSourceChip` drag + `GlowKnob::itemDropped` |
| Remove route | Right-click ring, or × on connection row |
| Live route list | `ModSourceStrip::paintOverChildren` |
| Wireframe preview | `ModRoutingWireframeView` (LFO curve, ENV curve, 3×2 node graph) |
| Scope follows engine selector | Global vs Engine N via `setRoutingContext` |
| Modal overlay | `ModRoutingOverlay` (M key, MOD tab launcher) |

### What the engine supports but PLAY UI does not expose

From `pw8::modulation::ModMatrixTypes.hpp`:

- **Sources:** 8 LFOs, 8 ENVs, velocity, pressure, aftertouch, MPE slide, 8 macros
- **Destinations:** Filter cutoff/resonance (global + per-operator), operator level, WT position, pan
- **Per-route fields:** `amount`, `scope` (Voice / Layer / Global for LFO routes)

PLAY mode intentionally limits assignable sources to **LFO 1, AMP ENV, Velocity** and destinations to **filter cutoff/resonance** — but patches can contain routes the UI cannot create or edit fully.

### Visual language (already established)

- Dual-pass **glow strokes** (`ObsidianDraw::strokeGlowPath`) — wavetable lines, knob arcs, chip outlines
- **Recessed panels** (`fillRecessedRoundedRect`) — cards, buttons, text fields
- **Source color identity** — LFO purple, ENV cyan, Velocity pink (`ObsidianPalette::modSourceColour`)
- **Wireframe tier** — pseudo-3D mesh, animated LFO/ENV previews, curved route links

---

## Operational Gaps (user-facing pain)

### P0 — Confusing / broken feel

1. **Connection list overlapped wireframe** — list painted full panel width instead of right column. **Fixed:** `connectionsArea_` tracks right column bounds.
2. **Help text on patches with existing routes** — hidden only after user creates a route in-session. **Fixed:** also hides when patch already has active routes.
3. **MOD tab is empty** — launcher only; users expect a matrix on MOD.
4. **No modulation depth visible** — `amount` stored but never shown. **Fixed (read-only):** amount column in connection list.
5. **Misleading copy** — referenced hidden PERF tab. **Fixed:** points to "Knobs of Interest".

### P1 — Incomplete matrix semantics

6. **Cannot edit amount** — assignment always uses `defaultModAmountFor()` (Cutoff ±36 st, Resonance ±0.4).
7. **Cannot edit scope** — always Voice scope on assign.
8. **Replace-on-assign** — one source per `(destination, targetIndex)`; multi-source summing exists in engine but UI implies single ring.
9. **Wireframe vs list disagree** — diagram shows 3×2 filter graph only; list shows all patch routes (Operator Level, Pan, etc.).
10. **Closing overlay disarms** — breaks "arm in matrix, assign on FILTER" if user closes early.

### P2 — Missing destinations / sources

11. Only 3 sources in palette — macros, other LFOs/ENVs invisible in PLAY.
12. Only 2 assignable destinations — Level, WT Pos, Pan require DESIGN mode or hand-authored `.pw8`.

---

## Visual Design Target

### Layout (recommended end state)

```
┌ MOD MATRIX — Global Filter ─────────────────────────────────────── × ┐
│ ① SOURCE   [LFO1] [AMP ENV] [VEL]     (armed chip glows)          │
│ ② TARGET   [Cutoff] [Resonance]       (enabled when armed)         │
├──────────────────────────────┬──────────────────────────────────────┤
│ LIVE PREVIEW                 │ ACTIVE ROUTES                        │
│ ┌ LFO waveform ────────────┐ │ ● LFO1 → Global Cutoff  [====●] +4.9×│
│ ├ ENV curve ───────────────┤ │ ● VEL  → Global Reso    [==●===] +0.3×│
│ └ Route graph (glow links) ┘ │                                      │
└──────────────────────────────┴──────────────────────────────────────┘
```

FILTER tab: compact source chip row + ringed Cutoff/Resonance knobs (no modal required for basic routing).

### Visual rules

| Element | Treatment |
|---|---|
| Armed source chip | Pulsing accent glow (`strokeGlowPath`, high alpha) |
| Destination buttons (step 2) | Recessed when disabled; accent glow outline when armed + enabled |
| Connection rows | Source dot color, recessed row bg when hovered/selected |
| Amount | Inline mini slider or drag bar; bipolar for cutoff (semitones) |
| Wireframe links | Thickness/alpha ∝ live mod output |
| Knob rings | Arc length ∝ normalized `amount / defaultMax` |
| Non-PLAY routes | Gray badge "patch only" — don't imply assignability |

---

## Implementation Phases

### Phase 1 — Layout & clarity (done / in progress)

- [x] Fix connection list column bounds
- [x] Show read-only amount in list
- [x] Numbered step copy (① source → ② destination)
- [x] Hide help when patch has routes
- [ ] Real `ModRouteRow` components (replace paint-only × buttons)
- [ ] Overlay header shows routing context ("Global Filter" vs "Engine 3 Filter")

### Phase 2 — Depth editing

- [ ] `assignModRoute(..., amount)` overload; preserve amount on replace
- [ ] Horizontal amount slider per row → `setOrReplaceModRouteLive` with new amount
- [ ] Post-assign depth popover under destination buttons
- [ ] Ring arc encodes depth on `GlowKnob`

### Phase 3 — MOD tab & FILTER integration

- [ ] Embed condensed matrix on MOD tab (not launcher-only)
- [ ] `ModSourcePalette` on FILTER tab (`setCompactLayout(true)`)
- [ ] Optional: keep armed state when overlay closes (with banner reminder)

### Phase 4 — Honest wireframe

- [ ] Dynamic node graph from active routes (not hard-coded 3×2)
- [ ] Or: label wireframe "Filter routes" + full list for everything else
- [ ] Animate link glow from live mod output

### Phase 5 — Expanded PLAY scope (product decision)

- [ ] Add Level / WT Pos / Pan to destination grid (engine-scoped)
- [ ] Add Macro 1–4 to source palette (most common performance sources)
- [ ] Scope pill per row (Voice / Layer) for LFO routes

---

## File Map

| File | Role |
|---|---|
| `plugin/src/ui/components/ModRoutingOverlay.*` | Modal shell |
| `plugin/src/ui/components/ModSourceStrip.*` | Matrix body: wireframe + palette + destinations + route list |
| `plugin/src/ui/components/ModLauncherPanel.*` | MOD tab gateway |
| `plugin/src/ui/components/ModAssignmentController.h` | Shared armed source |
| `plugin/src/ui/components/ModRoutingUi.*` | assign, labels, defaults, patch focus inference |
| `plugin/src/ui/components/ModSourcePalette.*` | Reusable chip row |
| `plugin/src/ui/components/ModSourceChip.*` | Chip + drag |
| `plugin/src/ui/components/GlowKnob.*` | Ringed mod targets |
| `plugin/src/ui/components/wireframe/ModRoutingWireframeView.*` | Left-panel preview |
| `plugin/src/ui/components/FilterLfoPanel.*` | FILTER tab mod targets |
| `plugin/src/ui/PlayModeEditor.*` | Orchestration, banner, M key |
| `engine/include/pw8/modulation/ModMatrixTypes.hpp` | Full route model |

---

## Success Criteria

1. **Operational:** User can assign, see, adjust depth, and remove every route they can create — without opening DESIGN mode.
2. **Visual:** Matrix feels as polished as wavetable wireframes — glow, depth, recessed panels, source color consistency.
3. **Honest:** UI never implies control over routes it cannot edit; patch-only routes are labeled.
4. **Discoverable:** Basic routing possible from FILTER tab; MOD tab shows useful matrix, not a stub.

---

## Related

- [UI.md](UI.md) — GATE 3 mod routing scope
- [MODULATION.md](MODULATION.md) — engine mod matrix semantics
- [PATCH_FORMAT.md](PATCH_FORMAT.md) — `uiFocus` for patch-authored performance knobs
