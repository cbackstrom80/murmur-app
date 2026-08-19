# Figma Knob Migration

**Status:** In progress (Aug 2026)  
**Figma source:** `glow-ring-knobs` frame `21:4` in MURMUR-Obsidian  
**Code Connect:** `plugin/src/ui/figma-connect/GlowKnob.figma.ts`, `ConcentricGlowKnob.figma.ts`, `TripleGlowKnob.figma.ts`

---

## Goal

Replace legacy flat Obsidian rotaries with the **Figma decked glow-ring knob** everywhere Ben touches the synth.

## Component map

| Figma | C++ | Use when |
|-------|-----|----------|
| Single glow-ring knob | `GlowKnob` | One APVTS param (default everywhere) |
| UX-09 double ring | `ConcentricGlowKnob` | Cutoff/res, coarse/fine pairs |
| UX-09 triple ring | `TripleGlowKnob` | Three related params (future) |

## What changed (1.4.5+)

1. **`GlowKnob` defaults to decked Figma style** — constructor calls `setDeckedStyle(true, Medium)` so panels that never opted in still match Figma.
2. **`FigmaKnobTokens.h`** — canonical diameters from `PlayModeLayout.h` (28 chrome master, 36 macro compact, 72 feature KOINS, etc.).
3. **Header compact knobs** — chrome master volume uses Small deck + 28px diameter token.

## Remaining work

| Priority | Task |
|----------|------|
| P0 | Tokenize stroke weights in `DeckedKnobDraw.h` from Figma specs (when Chris exports tokens) |
| P1 | Replace flat `EngineCard` pitch dials with live `ConcentricGlowKnob` or decked singles |
| P1 | Audit `FilterLfoPanel`, `GlobalPanel`, `ArpPanelOverlay` — set explicit diameters via `FigmaKnobTokens` |
| P2 | Migrate dual-param sites from two `GlowKnob`s → `ConcentricGlowKnob` |
| P2 | Remove legacy non-decked path in `ObsidianLookAndFeel::drawRotarySlider` once unused |
| P3 | Design FX detail knobs → decked `GlowKnob` via `applyFigmaContext(DesignFxDetail)` |

## For Figma handoff

When designing new screens (e.g. **Master Motion Lab**), use:

- **Large (88px)** — hero performance macros, master envelope ADSR
- **Medium (64–72px)** — panel grids, LFO rate/depth
- **Small (28–36px)** — chrome bar, compact strips

Export component variants: `Size=Small|Medium|Large`, `Featured=on|off`, `ModRing=on|off`.
