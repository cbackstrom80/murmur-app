# Compact Knob Research — Figma `21:4` → `4:1134`

**Purpose:** Map the **glow-ring-knobs reference sheet** (`21:4`) to **COMPACT macro knobs** (`4:1134`) and document code gaps.

**Specs:** [`glow-ring-knobs.21-4.layout.json`](../plugin/src/ui/figma-connect/layouts/glow-ring-knobs.21-4.layout.json) · [`murmur-compact-view.4-1134.layout.json`](../plugin/src/ui/figma-connect/layouts/murmur-compact-view.4-1134.layout.json)

---

## 1. What `21:4` is (verbatim metadata)

Frame **`glow-ring-knobs`** — **1280×720** @ y=7700. This is the **canonical decked knob anatomy board**, not the COMPACT shell itself.

### Hero knob anatomy (`knob-radial-frame` 180×180)

Used in OSC / FILTER containers (`21:74`, `21:102`):

| Layer | Node | Size | Inset from 180 frame | Ø ratio |
|-------|------|------|----------------------|---------|
| Outer track + accent | `21:75`–`76` | **160×160** | 10 | **0.889** |
| Inner track + accent | `21:77`–`78` | **124×124** | 28 | **0.689** |
| Value readout | `21:79` | **80×80** | 50 | **0.444** |

Center text example: `7.5` + `DB` (23px + 10px labels).

### Triple-ring variant (`21:130` — MODULATION ENVELOPE)

| Ring | Size | Inset |
|------|------|-------|
| Outer | 168 | 6 |
| Middle | 134 | 23 |
| Inner | 100 | 40 |
| Readout | 68 | 56 |

Maps to **`TripleGlowKnob`** (three stacked value arcs).

### Chrome master (`21:33`)

| Property | Figma |
|----------|-------|
| `knob-graphic` | **28×28** |
| Label | `MASTER` 8px below |

Maps to **`kChromeMasterKnobSize`** + `GlowKnob::setHeaderCompactMode(true)`.

### Sub-param sliders (below hero)

`knob-sub-params` @ y=249: horizontal sliders **6px** tall, label row **10px** — not yet a C++ component (future `KnobSubParamStrip`).

---

## 2. COMPACT macros (`4:1134`) — scaled application of `21:4`

From **`performance-macros`** (`4:1172`):

| Property | Figma (verbatim) |
|----------|------------------|
| Cell | **84×52** |
| `knob-graphic` | **36×36** @ x=24 in cell |
| Label row | **10px** @ y=42 |
| Grid | 3×2, col gap **8**, row gap **10** |
| Example labels | TEXTURE, WARMTH, MOTION, SPACE, BITE, SHIMMER |

**Scale factor** from hero to compact macro: **36 / 180 = 0.20** (20%).

If hero ratios hold at 36px:

| Ring | Predicted @ 36px |
|------|------------------|
| Outer | ~32px |
| Middle | ~25px |
| Cap | ~16px |

---

## 3. Code mapping today

| Surface | Figma Ø | Constant | C++ | Decked size |
|---------|---------|----------|-----|-------------|
| Chrome master | 28 | `kChromeMasterKnobSize` | `MurmurChromeBar` | Small |
| **Compact macro** | **36** | `kCompactMacroKnobSize` | `PatchFocusPanel` | **Small** |
| Compact master out | 48 | `kCompactMasterKnobSize` | `CompactModeEditor` | Small |
| Desktop macro touch | 92 | `kDesktopPlayModeMacroKnobTouchSize` | `PatchFocusPanel` | Large |
| Hero reference | 180 | — (design only) | `GlowKnob` Large | Large |

**Render path:** `GlowKnob` → `ObsidianLookAndFeel` → `decked::drawDeckedRotarySlider` (`DeckedKnobDraw.h`).

### `DeckedKnobDraw` geometry vs Figma hero (180px)

| Ring | Figma ratio | Code Medium (`computeGeometry`) |
|------|-------------|----------------------------------|
| Outer | 0.889 | 0.96 radius → **0.96** Ø ratio |
| Middle | 0.689 | 0.80 radius → **0.80** |
| Cap | 0.444 | 0.55 radius → **0.55** |

Code rings run **~7% larger** than Figma hero at each tier. Acceptable at hero scale; at **36px** the error is ~2–3px — noticeable.

**P0 gap:** Tokenize ring ratios from `21:4` in `computeGeometry` instead of hardcoded 0.96/0.80/0.55.

---

## 4. Bug found — compact dial cap was wrong

**Before fix:** `PatchFocusPanel::applyLayoutMode()` used **56px / 52px** caps for compact.

**Figma says:** **36×36** (`4:1176`).

That overshoot clipped the 84×52 cell and broke the 3×2 grid alignment from the layout pass.

**Fix:** Both feature + standard compact knobs → `layout::kCompactMacroKnobSize` (36) + `DeckedKnobSize::Small`.

---

## 5. Remaining compact knob gaps

| Gap | Figma | Code | Priority |
|-----|-------|------|----------|
| Label font | 10px row, ~8pt | `fonts::label(11)` default | P1 |
| Value readout in cap | numeric + unit in dial | slider text box hidden in compact | P2 (match hero readout) |
| Mod ring stroke | visible on hero | `KnobRingDraw` scales with diameter | OK |
| Featured macro warm tint | amber on KOINS | `setFeaturedPerformanceMacro` | OK |
| Triple/concentric | N/A in compact | N/A | — |

---

## 6. Recommended scale ladder (from `21:4`)

Use **`21:4` hero = 180px** as reference unit; derive all product sizes:

```
180  hero / design reference (21:4)
 92  desktop macro touch (36:4)
 48  compact master out (4:1226)
 44  vocoder / MI panels
 36  compact macro (4:1176) ← Small decked
 28  chrome master (21:33)
```

Always pair diameter with `figma::deckSizeForDiameter()`:
- ≤36 → **Small**
- 37–87 → **Medium**
- ≥88 → **Large**

---

## 7. Export pipeline (knob-specific)

1. Export **`21:4`** variant radii → `glow-ring-knobs.21-4.layout.json`
2. Add **`compactMacroDialDiameter()`** to `FigmaKnobTokens.h`
3. Never hardcode dial sizes in panels — use tokens
4. When Chris exports Figma variables for stroke weights, replace `trackThickness` heuristics in `DeckedKnobDraw.h`

**Related:** [`FIGMA_LAYOUT_EXPORT.md`](FIGMA_LAYOUT_EXPORT.md) · [`FIGMA_KNOB_MIGRATION.md`](FIGMA_KNOB_MIGRATION.md) · [`KNOB_RING_SEMANTICS.md`](KNOB_RING_SEMANTICS.md)
