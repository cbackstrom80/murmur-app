# QUASAR Return — Implementation Plan

**Date:** 2026-08-17  
**Figma canonical frame:** [`102:4` — `murmur-master-quasar-binaural`](https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=102-4)  
**Sprint:** **Sprint 8** — [MI_IMPLEMENTATION_SPRINT.md](MI_IMPLEMENTATION_SPRINT.md#sprint-8--quasar-in-murmur-binaural-spatializer)  
**Status:** **PLANNING** — strategic re-integration into MURMUR master bus (**in-MURMUR only**)  
**Audience:** Curtis + implementation agents  

> **Product decision (2026-08-17):** QUASAR is **not** a separate standalone plugin. It lives exclusively in MURMUR as a master FX slot + full-screen editor (`102:4`). The extracted `quasar_plugin/` tree is **legacy / do not ship**; `PW8_BUILD_QUASAR_PLUGIN` stays **OFF**. Interstellar Spatial presets embed Quasar params in `.pw8` (companion `.quasar` files are migration-only).

**Related:** [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) (historical in-MURMUR spec + DSP truth), [`QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md) *(superseded — archive reference only)*, [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md), [`MI_IMPLEMENTATION_SPRINT.md`](MI_IMPLEMENTATION_SPRINT.md), [`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md)

---

## Executive summary

**QUASAR is coming back inside MURMUR** — not as the old GLOBAL sub-tab, but as a **full-screen master FX experience** when the user selects QUASAR in the master bus chain. Figma `102:4` defines a **1280×720 hero spatializer** that is dramatically more ambitious than the current standalone `QuasarEditor` (920×720 wireframe + DEEP drawer).

| Decision | Choice |
|----------|--------|
| **Product shape** | **In-MURMUR only** — master FX slot + `102:4` hero panel; **no** standalone QUASAR app/VST |
| **UI entry** | PLAY Advanced → master chain breadcrumb → **QUASAR** pill active → full `102:4` panel |
| **DSP core** | `effects::BinauralSpaceProcessor` in `pw8_core` / master chain — **not** a forked plugin target |
| **Slot type** | Re-enable `EffectType::BinauralSpace` in master FX M1–M4 (legacy type 11 migration reversed for Spatial bank) |
| **Preset story** | Quasar params **embedded** in `.pw8` `masterEffects[]`; deprecate companion `.quasar` over time |
| **Cool factor** | Live azimuth ring, dual QSR splines, grain field overlay, phase correlation scope, HRTF profiles, telemetry HUD |

---

## What Figma `102:4` specifies

Frame name: **`murmur-master-quasar-binaural`** · **1280×720** · fits PLAY Advanced content area.

### Layout map

```
┌─ header-bar (44px) ─────────────────────────────────────────────────────────┐
│ MURMUR · 8-ENGINE BINAURAL SPATIALIZER                                      │
│ MASTER BUS CHAIN: [EQ][COMP][SATURATE][STEREO] › [QUASAR●]                  │
│                                    WIDE FIELD / BANK: MASTER SPATIAL [BYPASS]│
├─ binaural-visualization-panel (300px hero) ─────────────────────────────────┤
│ BINAURAL FIELD                    SPATIALIZATION / HRTF CROSSFEED           │
│ [IN meter]  azimuth ring + head + L/R ears + source dots + splines  [OUT]   │
│             compass: 0° FRONT · 180° REAR · 270° LEFT · 90° RIGHT           │
│                                                      [ELEV slider + readout] │
├─ binaural-engine-knobs-row (7 × 44px knobs) ────────────────────────────────┤
│ WIDTH · CROSSFEED · DISTANCE · AZIMUTH · ELEVATION · HRTF SIZE · MIX        │
├─ bottom-controls (flex) ────────────────────────────────────────────────────┤
│ BINAURAL ENGINE card          │  SPATIAL & CHAIN POSITION card              │
│ HRTF: KEMAR CIPIC CUSTOM●     │  ROOM: NONE SMALL MEDIUM● HALL              │
│ MODE: BINAURAL● MID-SIDE STEREO│  Early Refs · Air Absorption · HP Comp     │
│ Mono Below · Phase Corr meter │  ‹ BACK TO FX CHAIN · BYPASS QUASAR         │
│ CPU · LATENCY · GRAINS HUD    │  correlation scope (waveform)               │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Color roles (from Figma)

| Role | Hex | Use |
|------|-----|-----|
| Cyan accent | `#00c8ff` | QSR1 / IN meter / L ear |
| Violet accent | `#7c4dff` / `#e040fb` | QSR2 / OUT meter / QUASAR pill |
| Panel base | `#07080c` → `#12151c` gradient | Hero + cards |
| Bypass danger | `#e07f7f` on `#3b1c1c` | BYPASS actions |

Reuse Obsidian tokens from `ObsidianPalette.h`; add **`kQuasarViolet`** if not present.

---

## Current codebase vs Figma gap

| Figma `102:4` feature | Today | Gap |
|------------------------|-------|-----|
| Master chain breadcrumb | `GlobalPanel` + `FxChainFlowView` (no QUASAR pill) | Wire QUASAR type + highlight |
| Azimuth ring hero | `QuasarSpatialWireframeView` (top-down floor grid) | **New** `QuasarBinauralFieldView` — polar ring, splines, compass |
| 7 primary knobs | Standalone: 6 PLAY + macros; no WIDTH/HRTF SIZE | Map + add params |
| HRTF profiles | ITD/ILD only (`BinauralPanner.hpp`) | KEMAR/CIPIC/CUSTOM switch + ear-size scaling |
| Processing modes | Headphone/Speaker/Auto only | BINAURAL / MID-SIDE / STEREO ENHANCE |
| Room presets | Per-QSR room amount/size/damping (DEEP) | Surface as NONE/SMALL/MEDIUM/HALL macro |
| Early refs + air | Not implemented | New early-ref tap + HF air absorption |
| HP compensation | Crossfeed only | Profile dropdown (MDR-7506, etc.) |
| Phase correlation | Not in UI | Meter + mini scope (reuse oscilloscope patterns) |
| GRAINS ACTIVE | N/A | Tie to Clouds granular engine (Track F) or delay grains |
| Telemetry HUD | N/A | CPU %, reported latency from `EffectLatency.hpp` |
| Full-screen in MURMUR | Removed (standalone pivot) | **Restore** as overlay route |

**Assets already built:** `BinauralSpaceProcessor`, 75 Interstellar `.quasar` companions, 20 PLAY `.quasar` presets, `QuasarSpatialMacros.hpp`, mod-matrix history (removed — restore selectively).

---

## Architecture

### Signal flow (unchanged core, extended params)

```
voices → layer inserts → stereo sum (+ masterGain)
  → master FX chain M1–M4
      └── [QUASAR / BinauralSpace]  ← insert point
            ├── QSR1 (L or mono) → binaural pan + room
            ├── QSR2 (R or aux)  → binaural pan + room
            ├── CNTR anchor
            ├── early refs + air (new)
            └── post-sum delay
  → limiter / remaining slots → DAW out
```

Sidechain: reuse AU sidechain bus → QSR2 aux (already in standalone processor).

### Shared component strategy

Build **once in `plugin/src/ui/components/quasar/`** — MURMUR-only (no `quasar_plugin/` port):

| Component | Owner |
|-----------|--------|
| `QuasarBinauralFieldView` | `MasterQuasarPanel` |
| `QuasarPrimaryKnobRow` | `MasterQuasarPanel` |
| `QuasarEngineCard` | `MasterQuasarPanel` |
| `QuasarSpatialCard` | `MasterQuasarPanel` |
| `QuasarChainHeader` | `MasterQuasarPanel` |
| `QuasarTelemetryBar` | `MasterQuasarPanel` |

**Code Connect:** add `MurmurMasterQuasarBinaural.figma.ts` → frame `102:4`.

### Navigation / state

1. User opens PLAY → **GLOBAL** (or OUTPUT context) → selects master slot with type QUASAR  
2. `PlayModeEditor` pushes **`PlaySubView::MasterQuasar`** (new enum) — hides engine grid, shows `102:4` panel full bleed  
3. **‹ BACK TO FX CHAIN** → `PlaySubView::Global` or previous sub-view  
4. **BYPASS QUASAR** → slot bypass APVTS (not global plugin bypass)  
5. Header **BYPASS** (red) → global master FX bypass or whole Quasar wet mix = 0 (TBD — recommend slot bypass)

Persist last sub-view in `PluginState` for session restore.

### Schema (`masterEffects[i]` when `type == BinauralSpace`)

Extend `EffectSlotParams` Quasar block (backward compatible defaults):

```jsonc
"masterEffects": [{
  "type": "binaural_space",
  "bypass": false,
  "mix": 0.85,
  "quasar": {
    // existing QSR1/2/CNTR spatial + room + delay (28 params)
    "width": 1.35,              // NEW — stereo enhancement / spread macro
    "hrtfProfile": "cipic",     // kemar | cipic | custom
    "hrtfSize": 0.5,            // 0..1 → Small..Large (Neuzeit Ear Type)
    "processMode": "binaural",  // binaural | midSide | stereoEnhance
    "monoBelowHz": 200,
    "roomPreset": "medium",     // none | small | medium | hall → fans to room FDN
    "earlyReflections": 0.6,
    "airAbsorptionM": 8.0,
    "airAbsorptionOn": true,
    "hpCompProfile": "mdr7506", // off | mdr7506 | hd650 | custom
    "hpCompOn": true
  }
}]
```

MCP + `PatchSerializer` roundtrip required before factory Spatial bank re-embeds Quasar in-slot.

---

## Phased implementation

### Phase Q0 — Re-integration scaffold (≈2 weeks)

**Goal:** QUASAR audible in MURMUR master chain again; minimal UI.

| Task | Owner | Verify |
|------|-------|--------|
| Re-add `EffectType::BinauralSpace` to enum + APVTS master slot types | `EffectTypes.hpp`, `PluginState` | Select QUASAR on M3; passes audio |
| Master chain processor calls `BinauralSpaceProcessor` | `MasterFxChain` / `Engine.cpp` | `ctest` DSP smoke |
| Reverse Spatial preset migration: optional in-slot Quasar vs companion-only | preset loader + metadata | Init + one Spatial preset |
| `MasterQuasarPanel` shell — header + bypass + mix knob only | `MasterQuasarPanel.*`, `PlayModeEditor` | OPEN from GLOBAL chain |
| Doc + audit registry row for `102:4` | this doc, `FIGMA_UI_AUDIT.md` | Frame linked |

**Exit:** Build green; QUASAR slot processes stereo bus; bypass safe.

---

### Phase Q1 — Hero UI pixel-match `102:4` (≈3 weeks)

**Goal:** The “wow” screen — azimuth ring + meters + 7 knobs.

| Task | Figma node | C++ | Verify |
|------|------------|-----|--------|
| `QuasarBinauralFieldView` — ring, compass, head, L/R, draggable QSR dots | `102:50`, `105:*` | new component | Drag dot → azimuth/elevation APVTS |
| IN/OUT segmented LED meters | `102:34`, `102:227` | reuse VU segment pattern | Meters animate on audio |
| ELEV vertical slider | `105:25` | `juce::Slider` vertical | Linked to elevation param |
| Spline motion trails (LFO/mod orbit preview) | `105:89` | timer + path cache | Orbit macro moves splines |
| `QuasarPrimaryKnobRow` — 7 GlowKnobs | `102:243` | map to APVTS | Ghost mod pointers (MURMUR 1.1.2) |
| `QuasarChainHeader` — breadcrumb + preset + bypass | `102:5` | extends `FxChainFlowView` | QUASAR pill glows violet |
| `PlaySubView::MasterQuasar` routing | — | `PlayModeEditor` | BACK returns to chain |
| Code Connect stub | `102:4` | `MurmurMasterQuasarBinaural.figma.ts` | Registry updated |

**Exit:** Side-by-side screenshot vs Figma ≤4px delta on hero + knob row.

---

### Phase Q2 — Binaural Engine card (≈2 weeks)

**Goal:** HRTF profiles, processing modes, phase safety.

| Task | Detail |
|------|--------|
| HRTF profile pills | Switch coefficient sets in `BinauralPanner` (KEMAR/CIPIC tables; CUSTOM = user IR folder post-MVP) |
| HRTF SIZE knob | Scales ITD/ILD + pinna filter freqs (Neuzeit “Ear Type”) |
| Mode pills | BINAURAL = current; MID-SIDE = encode/decode width; STEREO ENHANCE = M/S widen + crossfeed |
| Mono Below slider | HPF on CNTR path (existing `cntrHpfHz` — expose with new label) |
| Phase Corr meter | Running L/R correlation −1..+1 |
| Correlation scope | Mini waveform — reuse `OscilloscopeView` reduced |

**Exit:** Mode switch audible; phase meter responds to mono vs wide content.

---

### Phase Q3 — Spatial & chain card (≈2 weeks)

**Goal:** Room macro, air, headphone compensation.

| Task | Detail |
|------|--------|
| Room size pills | Map to `{amount, size, damping}` tuples per QSR (factory tuned) |
| Early reflections | Short multi-tap predelay before FDN room |
| Air absorption | Distance-dependent HF shelf on QSR paths |
| HP Comp toggle + profile | EQ curve presets for common headphones |
| BYPASS QUASAR + BACK buttons | Wire to slot bypass + nav |

**Exit:** ROOM pill changes tail; air slider darkens HF at distance.

---

### Phase Q4 — Performance, mod, 8-engine tie-in (≈2 weeks)

**Goal:** Make it feel alive and integrated with MURMUR’s synth identity.

| Task | Detail |
|------|--------|
| Telemetry bar | `CPU ALLOCATION` from processor meter; `LATENCY` from reported ms; `GRAINS` from active Clouds/granular voices |
| **Grain field overlay** | When granular engine active, render grain positions as faint dots in azimuth ring (optional toggle) |
| Restore mod destinations | `QuasarMix`, QSR angles, room, delay — up to 18 from historical plan |
| Morph KOIN paths | Spatial params in morph keyframes (Spatial bank already has easing migration) |
| SPACE macro KOIN | Basic/Compact: ORBIT + SPREAD fan-out (reuse `QuasarSpatialMacros.hpp`) |
| Companion sync button | “Load `.quasar` companion” from patch metadata one-click |

**Exit:** LFO → azimuth visible on ring; SPACE macro moves scene in Basic view.

---

### Phase Q5 — Factory + preset migration (≈1 week)

| Task | Detail |
|------|--------|
| `MASTER SPATIAL` factory bank | 20+ in-MURMUR presets with Quasar on master bus |
| Re-embed Quasar in 75 Spatial `.pw8` | Script: `scripts/embed_spatial_quasar_slot.py` — absorb companion `.quasar` JSON into slot |
| Remove / archive `quasar_plugin/` from release pipeline | `PW8_BUILD_QUASAR_PLUGIN=OFF` permanently; docs point to in-MURMUR path |

---

## “Really cool” differentiators

These are the features that go beyond Neuzeit parity and justify the `102:4` layout:

1. **Live dual splines** — Cyan (QSR1) and violet (QSR2) trails show recent azimuth motion; brighten under mod/LFO.
2. **Interactive azimuth ring** — Drag sources on the ring; elevation on vertical slider; distance scales dot radius.
3. **Grain constellation** — Granular engine grains appear as a star field in the binaural viewport (8-engine story: “spatializer for the whole patch”).
4. **Phase correlation scope** — Immediate mono-compatibility feedback for live/streaming (cathedral pads stay safe).
5. **HRTF morph** — Crossfade KEMAR ↔ CIPIC with SIZE knob for “elephant ear” moments without leaving binaural mode.
6. **Chain-aware header** — Breadcrumb shows real M1–M4 types from patch; click sibling pill to jump slot editor.
7. **Mod ghost rings** — All 7 primary knobs show live modulated value (existing MURMUR affordance).
8. **One patch, one scene** — Spatial character ships entirely inside the `.pw8`; no external plugin required.

---

## Position in MI sprint program

| Sprint / track | Relationship to QUASAR return |
|----------------|-------------------------------|
| Sprint 4 Streams (C) | Master chain neighbor — sidechain feeds QUASAR QSR2 aux |
| Sprint 5 Stages (D) | Segment envelopes can mod spatial params |
| Sprint 6 Marbles (E) | Random voltage → azimuth/distance |
| Sprint 7 Clouds (F) | **GRAINS ACTIVE** HUD + grain field overlay |
| Spatial factory bank | Primary QA target — 75 presets + companions |

**Recommended slot in program:** Start **Q0 immediately after Sprint 4 Streams** (shared master-chain work). Run **Q1 hero UI in parallel** with Sprint 5 if design assets stable.

---

## Risks & mitigations

| Risk | Mitigation |
|------|------------|
| CPU: HRTF × 2 paths + 8 engines | Quality tier: Normal = ITD/ILD; High = optional HRIR; telemetry warns |
| Latency reporting | Centralize in `EffectLatency.hpp`; show in HUD |
| Preset backward compat | Default `enabled: false`; companion `.quasar` still works; legacy type 11 mapping table |
| Standalone vs in-MURMUR drift | Shared `quasar/` UI folder + single `BinauralSpaceParams` struct |
| Figma scope creep | Phase Q1 locks hero; engine/spatial cards Q2/Q3 |

---

## Definition of done (QUASAR return)

1. Figma `102:4` registered in `FIGMA_UI_AUDIT.md` with C++ owner `MasterQuasarPanel`.
2. QUASAR selectable on master bus; bypass-safe; Spatial preset plays without external plugin (in-slot mode).
3. Hero viewport + 7 knobs pixel-match; host visual QA recorded.
4. At least one HRTF profile + room preset audible difference.
5. No standalone QUASAR target in CI or release packages.
6. `ctest` covers serializer roundtrip + `BinauralSpaceProcessor` mode switch.

---

## Immediate next actions

1. Add **`102:4`** to Figma Part 6 registry + Code Connect stub.
2. Spike **`QuasarBinauralFieldView`** — azimuth ring paint + drag → `qsr1AngleDeg` / `qsr2AngleDeg`.
3. Re-enable **`EffectType::BinauralSpace`** in master type row (`GlobalPanel` currently lists through VOCODER only).
4. Schedule **Q0** sprint kickoff after Streams master-chain merge lands.

---

*Plan authored from Figma MCP pull of node `102:4` and repo audit of `quasar_plugin/`, `BinauralSpace.hpp`, and Spatial preset metadata.*
