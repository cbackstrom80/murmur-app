# Filter Routing Spec (Blades / Track B)

**Date:** 2026-08-17  
**Status:** Shipped (B-M2 DSP, B-M3 UI, B-M4 Filter Lab)  
**Related:** [`MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md`](MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md) §6, [`MI_IMPLEMENTATION_SPRINT.md`](MI_IMPLEMENTATION_SPRINT.md) Sprint 1, [`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md) Part 6 (`89:313`, `89:5`, `89:246`)

---

## 1. Overview

Dual-filter **routing morph** blends three topologies as `filterRouting` sweeps **0 → 1**:

| `routing` | Topology | Label (UI) |
|-----------|----------|------------|
| **0.0** | Serial F1 → F2 | Serial |
| **0.5** | Parallel sum of F1 and F2 | Parallel |
| **1.0** | Crossfade of serial F1→F2 and serial F2→F1 | Crossfade |

Filter 1 is the SVF (`StateVariableFilter`); Filter 2 is the character ladder (`CharacterFilter`). Mode morph (LP→BP→HP) is independent on F1 via `filterModeMorph`.

---

## 2. DSP (`FilterRouting.hpp`)

Implementation: `engine/include/pw8/filter/FilterRouting.hpp`

```cpp
// r ∈ [0, 1]
serialF1F2 = F2(F1(x))
serialF2F1 = F1(F2(x))
parallel    = 0.5 * (F1(x) + F2(x))
crossfade   = 0.5 * (serialF1F2 + serialF2F1)

if r <= 0.5:
    t = r * 2
    out = serialF1F2 + (parallel - serialF1F2) * t
else:
    t = (r - 0.5) * 2
    out = parallel + (crossfade - parallel) * t
```

**F2 cutoff tracking** when F1 enabled:

```
f2CutoffHz = f1ModulatedCutoffHz * 2^(cutoffOffsetSemis / 12)
```

When F1 disabled, F2 uses absolute `filter2.cutoffHz` with key track.

**Voice integration:** `engine/include/pw8/voice/Voice.hpp` calls `applyDualFilterRouting` with live `filterRouting` + mod offsets.

---

## 3. Patch / APVTS fields

| Field | APVTS ID | Range | Notes |
|-------|----------|-------|-------|
| `layerA.filterRouting` | `filterRouting` | 0..1 | Routing morph |
| `filter1.modeMorph` | `filterModeMorph` | 0..1 | LP→BP→HP on SVF |
| `filter2.cutoffOffsetSemitones` | `filter2CutoffOffsetSemis` | semis | Relative to F1 modulated cutoff |
| `filter2.drive` | `filter2Drive` | 0..1 | Character filter drive |

Live sync: `Engine::setFilterRoutingLive()` in `PatchworkEightProcessor`.

---

## 4. Mod matrix destinations

| Destination | Enum | Effect |
|-------------|------|--------|
| Filter Mode Morph | `FilterModeMorph` (25) | Additive offset on F1 mode morph |
| Filter Routing | `FilterRouting` (26) | Additive offset on routing morph |
| Filter 2 Drive | `FilterDrive` (27) | Additive offset on F2 drive |

Defined in `engine/include/pw8/modulation/ModMatrixTypes.hpp` (appended after `MorphPosition` = 24 for preset compatibility).

---

## 5. UI surfaces

| Surface | Figma | C++ |
|---------|-------|-----|
| PLAY Blades row | `89:5` / `89:131` | `FilterLfoPanel` — 7 knobs, mod rings on ROUTE/MORPH/DRIVE |
| Routing diagram | `89:246` | `FilterRoutingWireframeView` — 3-state stack, opacity morph |
| DESIGN Filter Lab | `89:313` | `DesignFilterLabPanel` — 420/524/280 columns, hero ROUTING 64px |

**Navigation:** DESIGN → FILTER (chrome sub-nav or status bar chip). **OPEN IN PLAY FILTER** → PLAY Advanced FILTER tab.

Layout constants: `plugin/src/ui/PlayModeLayout.h` (`kPlayBlades*`, `kDesignFilterLab*`, `kFilterRoutingWireframe*`).

---

## 6. Tests

| Test | Path |
|------|------|
| Routing topology | `tests/dsp/FilterRoutingTests.cpp` |
| Mod dest wiring | `tests/unit/ModMatrixTests.cpp` (Blades destinations) |

Run: `ctest -R FilterRouting`

---

## 7. Open items (post Sprint 1)

- Filter 1/2 **pre-drive** staging before each filter (optional B-M1 polish)
- SVF **resonance level compensation** at high Q (self-osc stability)
- Golden preset hash regen if filter DSP changes affect factory Blades bank
