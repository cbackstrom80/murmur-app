# UI Graphics Plan — Wavetable & Granular Visualizers

**Date:** 2026-08-17  
**Scope:** Restore pre-Obsidian wavetable mesh + granular overlays; document gaps for Figma-sized surfaces.

---

## Inventory — what existed

| Asset / renderer | Location | Technique | Data source |
|------------------|----------|-----------|-------------|
| **WavetableStackView** | `plugin/src/ui/components/WavetableStackView.{h,cpp}` | Pseudo-3D depth mesh (painter's algorithm, glow strokes) | `PatchworkEightProcessor::getActiveWavetableTable()` — real table frames + warp |
| **Granular grain windows** | `WavetableStackView::paint()` / `setGranularOverlay(true)` | Warm semi-transparent rects over mesh | APVTS `WavetablePos`, `GrainSizeMs` |
| **WireframeProjection** | `plugin/src/ui/components/wireframe/WireframeProjection.h` | Shared `paintDepthMesh`, `paintFlatWaveform`, `paintBarLandscape` | Callable sample fn |
| **Engine wireframes** | `EngineWireframeViews.{h,cpp}` | Classic/FM/Additive/Phase/Resonator/Noise previews | `OscPreviewSampler` — procedural, not audio tap |
| **OscWireframeHost** | `wireframe/OscWireframeHost.{h,cpp}` | Swaps engine-specific child views | Used by `OperatorEditorPanel` (PLAY) |
| **WavetableLabPanel mesh** | `WavetableLabPanel.cpp` | Embeds full `WavetableStackView` in center column | Same as stack view |
| **Design v2 context viz** | `EngineOscillatorPicker.cpp` | Per-engine paint paths in 80px strip | Was **placeholder** for WT (classic sine mesh); GRN (procedural cloud only) |

No binary PNG/SVG wavetable assets, shaders, or cached render images exist — everything is **procedural C++ paint**.

Git history: mesh upgraded in `a0d2fb2` (UI GATE 6); original stack in `e15004d` (UI GATE 5). See `docs/VISUALIZATION_UI_GATE5.md`.

---

## Fit assessment vs Figma sizes

| Surface | Figma / layout size | Aspect | Existing renderer fit |
|---------|---------------------|--------|------------------------|
| Engine card context viz | **80px** tall × ~**279px** wide (`kDesignModeV2ContextVisualizerHeight`, card 303 − padding) | ~3.5:1 wide | **Yes** — `contextThumbMeshOptions()` (9 rows, tighter stepY) |
| Wavetable lab hero mesh | Center column, ~**400×300+** (fluid) | ~4:3 | **Yes** — `labHeroMeshOptions()` (15 rows, original tuning) |
| Harmonic editor | **256×336** | ~3:4 tall | **Partial** — bar chart is seed/static; needs live partial data |
| Frame strip minis | **70×50** each × 8 | ~7:5 | **Gap** — sine placeholder waves; should sample real frames |
| Play-board compact stubs | **32×10** | — | **By design** — non-interactive chrome stubs only |

---

## Wired back (2026-08-17)

1. **`WavetableMeshPaint.{h,cpp}`** — shared real-table mesh + granular overlay extracted from `WavetableStackView`.
2. **`WavetableStackView`** — refactored to call shared painter (`labHeroMeshOptions()`).
3. **`EngineOscillatorPicker::paintContextWavetable`** — real wavetable mesh via `contextThumbMeshOptions()` + index marker.
4. **`EngineOscillatorPicker::paintContextGranular`** — real mesh + `paintGranularGrainOverlay()` + grain-cloud scatter dots.
5. **`WavetableLabPanel`** — unchanged wiring; still hosts full `WavetableStackView` (now uses shared painter internally).

---

## Still needs generation / improvement

Priority order:

| P | Surface | Size | Approach | Notes |
|---|---------|------|----------|-------|
| ~~1~~ | ~~Frame strip minis~~ | 70×50 | **DONE** — `paintWavetableFrameWaveform()` samples real frames | `WavetableLabPanel::paintFrameStrip` |
| ~~2~~ | ~~Harmonic editor bars~~ | 256×336 | **DONE** — `computeWavetableHarmonicMagnitudes()` from current frame | Refreshed @ 8 Hz in lab panel timer |
| ~~3~~ | ~~Legacy 4-cell WT/GRN grid~~ | ~36×14 cells | **DONE** — `paintWavetableWaveCell()` + granular overlay | `EngineOscillatorPicker::paintContextGrid` |
| 4 | Play-board osc stubs | 32×10 | **Static chrome** — no runtime render needed | Figma 22:44 intentionally minimal |
| 5 | Optional PNG export | any | **Runtime render-to-image** — only if marketing/docs need stills | Not required for plugin UI |
| ~~6~~ | Extract `EngineOscContextThumb` component | 279×80 | **DONE** — all 9 per-engine painters live in `EngineOscContextThumb.cpp`; picker syncs via `EngineOscContextPreviewData` | Figma screenshot pass next |
| 7 | Figma screenshot acceptance | 279×80 × 9 | Diff vs `28:921`–`28:1605` | Manual QA pass |

**Generation policy:** Prefer runtime procedural C++ (same data path as audio). Avoid shipping PNG/SVG wavetable assets — tables are JSON + live load.

---

---

## Per-OSC-type context thumb assets (Engine Card)

**Goal:** One JUCE-themed, Obsidian-wireframe visual per engine type that fits the design engine card context slot — live, param-driven, no raster atlas required.

### Target slot (design v2)

| Property | Value | Token / owner |
|----------|-------|---------------|
| Card outer | 303 × 270 | `kDesignModeV2CardWidth/Height` |
| Card padding | 12 | `kDesignModeV2CardPadding` |
| Context frame | full picker width × **80** | `kDesignModeV2ContextVisualizerHeight` |
| Usable paint area | ~**279 × 66** (after 1px frame + 6–8px inset) | `EngineOscillatorPicker::contextPreviewBounds_` |
| Frame | `#0A1114` fill, 6px corner, 1px border @ 85% | `paintContextVisualizerFrame()` |
| Caption row | 10px bottom strip, 7px label | `fonts::label(7)` · `palette::kTextDim` |
| Aspect | ~**4.2 : 1** wide | Figma cards under `37:787` |

**Not in scope for full thumbs:** play-board compact stubs (`22:44`) — keep 32×10 chrome badges only.

### Visual language (JUCE / Obsidian)

All nine thumbs share one family so cards read as a set:

| Token | Use in context thumbs |
|-------|------------------------|
| `palette::kAccent` (`#00FFD0`) | Primary strokes, live mesh rows, waveform glow |
| `palette::kAccentDim` | FM operator boxes, inactive partial bars |
| `palette::kMurmurViolet` | Granular grain windows, PHS phase marker |
| `palette::kAccentWarm` | FM feedback arc |
| `palette::kBackgroundBottom` | Frame fill, grid backdrops |
| `palette::kBorder` @ 22–35% | Grid lines, centre axis, dash guides |
| `draw::strokeGlowPath` / `wireframe::strokeGlowPath` | Live engine mesh + waveforms when `engineLive_` |
| `wireframe::paintDepthMesh` | Shared pseudo-3D projection (WT, GRN, lab hero) |
| `wireframe::paintFlatWaveform` | CLS, PHS output trace |
| `wireframe::paintBarLandscape` | ADD partial towers (landscape bars in wide slot) |
| Motion | `animPhase_` + `motionGain_` @ ~15 Hz when voice active |

Monoline 16×16 engine **icons** (`ENGINE_ICONS.md`, `EngineIconGrid`) remain separate from these **80px context thumbs** — icons label type; thumbs show parameter state.

### Figma reference nodes

| Batch | Node | Engines |
|-------|------|---------|
| A | `28:921` | CLS, WT, FM |
| B | `28:1281` | ADD, PHS, GRN |
| C | `28:1605` | NSE, RES, EXT |
| Card assembly | `37:787` | All 8 cards in design grid (verify thumb placement in situ) |
| Deep editor | `28:4` | Full-size wireframe column — not scaled thumbs |

Acceptance: screenshot diff each thumb @ **279×80** against Figma export from the nodes above (1280×720 frame, one card cropped).

---

### Per-type asset specification

Each row is the **canonical thumb** for that engine inside `EngineCard` → `EngineOscillatorPicker`.

| ID | Engine | Visual metaphor | Renderer (today) | Live data (APVTS / engine) | Status | Gap / polish |
|----|--------|-----------------|------------------|----------------------------|--------|--------------|
| **CLS** | Classic | Centre axis + glowing cycle trace; SQR uses zigzag path | `paintContextClassic` → `paintFlatWaveform` | `Waveform` 0–3 | **WIRED** | Match Figma stroke weight; sub-picker waveform sync |
| **WT** | Wavetable | Depth mesh stack + vertical index marker | `paintContextWavetable` → `contextThumbMeshOptions()` | `WavetablePos`, warp params, active table | **WIRED** | Empty-state copy; marker label alignment vs Figma |
| **FM** | FM/PM | OP4→OP1 boxes + feedback arc + ratio/index caption | `paintContextFm` (custom layout, not `FmWireframeView`) | `FmModulatorRatio`, `FmModulatorIndex`, `FmModulatorWaveform` | **WIRED** | Consider dual-layer mini mesh behind boxes like `FmWireframeView` |
| **ADD** | Additive | Landscape partial towers + tilt envelope | `paintContextAdditive` | Partial count / heights via `OscPreviewSampler` | **WIRED** | Drive heights from real partial gains when available |
| **PHS** | PhaseShape | 4×4 grid + thick distortion curve + output wave | `paintContextPhaseShape` | Phase-shape preset index | **WIRED** | Violet marker position vs sub-picker pill |
| **GRN** | Granular | WT mesh + grain windows + scatter cloud + centre line | `paintContextGranular` → mesh + `paintGranularGrainOverlay` | `WavetablePos`, `GrainSizeMs` | **WIRED** | Tune dot count/alpha for 80px; match Figma warm/violet mix |
| **NSE** | Noise/Chaos | Spectral slope fill + freq axis labels | `paintContextNoise` | Noise variant / spectral slope from sub-picker | **WIRED** | Optional live noise trace overlay from `NoiseWireframeView` |
| **RES** | Resonator | Peak spikes + envelope on baseline | `paintContextResonator` | Mode heights via `OscPreviewSampler` | **WIRED** | Spike width vs Figma; live mode count |
| **EXT** | External | Input pills (A/B/C/D) + env-follow level bar | `paintExternal` | `ExternalInputSource` 0–3 | **WIRED** | RESMP path still mono sidechain; host resample TBD |

**Policy:** Do **not** ship nine PNG/SVG atlases for runtime UI. Procedural C++ paint keeps thumbs in sync with loaded wavetables and APVTS. Export PNG only for marketing (`renderComponentToImage` one-off).

---

### Architecture — `EngineOscContextThumb`

Today all nine variants live as `paintContext*` methods on `EngineOscillatorPicker` (~400 lines). Recommended refactor when polishing pixels:

```
EngineOscillatorPicker
  └── EngineOscContextThumb (Component, 279×80)
        ├── ContextThumbFrame (shared border + caption strip)
        └── IContextThumbPainter (strategy)
              ├── ClassicThumbPainter
              ├── WavetableThumbPainter   ← WavetableMeshPaint + contextThumbMeshOptions
              ├── GranularThumbPainter      ← wraps Wavetable + GranularOverlay
              ├── FmThumbPainter
              ├── AdditiveThumbPainter
              ├── PhaseShapeThumbPainter
              ├── NoiseThumbPainter
              ├── ResonatorThumbPainter
              └── ExternalThumbPainter
```

| Approach | Pros | Cons | When |
|----------|------|------|------|
| **A. Inline paint** (current) | Zero extra components; fast to ship | Large picker cpp; harder to unit-test layout | Now — already shipped |
| **B. Extract thumb `Component`** | Isolated bounds; reusable in deep overlay mini preview | One child per card × 8 engines = 8 components | Phase 2d pixel polish |
| **C. Embed `OscWireframeHost` scaled** | Reuses full wireframes | Captions, arrows, min height ~120px — **does not fit 80px** | **Reject** for card slot |
| **D. Render-to-image cache** | Cheap repaint for static marketing | Stale when params animate | Docs / store screenshots only |

**Shared sizing helper** (already exists):

```cpp
// wireframe/WavetableMeshPaint.h
WavetableMeshPaintOptions contextThumbMeshOptions(); // 9 rows, tight stepY for 80px
```

Add parallel presets when extracting painters:

| Preset | Rows | Points/row | Intended bounds |
|--------|------|------------|-----------------|
| `contextThumbMeshOptions()` | 9 | 48 | 279×66 |
| `labHeroMeshOptions()` | 15 | 48 | ~400×300 |
| `deepEditorMeshOptions()` *(new)* | 12 | 48 | ~320×180 in `28:4` OSC column |

---

### Implementation roadmap (per-OSC assets)

| P | Task | Owner files | Depends on |
|---|------|-------------|------------|
| **0** | ✅ Restore WT mesh + GRN overlay in 80px strip | `WavetableMeshPaint.*`, `EngineOscillatorPicker.cpp` | — |
| **1** | Screenshot acceptance pass vs Figma `28:921` / `1281` / `1605` | audit doc + PNG diffs | Stable `37:787` layout |
| **2** | Extract `EngineOscContextThumb` + frame component | new `EngineOscContextThumb.{h,cpp}` | P1 diffs logged |
| **3** | FM thumb: optional mini dual-layer mesh under OP boxes | `FmThumbPainter`, reuse `FmWireframeView` sampling | P2 |
| **4** | ADD/RES: bind bar heights to engine partial/mode state (not preview seed only) | `OscPreviewSampler` + engine read API | Engine param expose |
| **5** | NSE: blend spectral curve + short live trace | `NoiseWireframeView` sampler | P2 |
| **6** | EXT: envelope follower from sidechain tap | `SidechainFollower`, `paintContextExternal` | EXT DSP routing |
| **7** | Legacy 4-cell grid (non–design-v2): reuse thumb painters scaled to 36×14 cells | `EngineOscillatorPicker` legacy layout | P2 |
| **8** | Optional `deepEditorMeshOptions()` for `EngineDetailOverlay` / `OperatorEditorPanel` | `OscWireframeHost` | Deep editor polish |

**Refresh rate:** Match existing wireframe timers — **15 Hz** preview refresh when panel visible; skip mesh rebuild when wavetable table pointer unchanged.

---

### Size matrix (where thumbs appear)

| Surface | Thumb size | Implementation |
|---------|------------|----------------|
| Design engine card | 279×80 | `EngineOscillatorPicker` design v2 |
| Engine deep overlay (`28:4`) | Full OSC column | `OscWireframeHost` + full `*WireframeView` |
| Wavetable lab | Hero mesh + 70×50 frame strip | `WavetableLabPanel` + `WavetableStackView` |
| Play board card | None (badge only) | `EngineCard::setPlayBoardCompactMode(true)` |
| Compact / Basic PLAY | None | Oscilloscope + patch focus only |

---

### Generator checklist (if Figma ≠ code)

Use this when a Figma thumb cannot be matched by scaling existing painters:

1. Capture Figma export @2x for the single context frame (not whole card).
2. Identify primitive: mesh / waveform / bars / spectrum / pills.
3. Map to existing `wireframe::*` paint function before writing new C++.
4. Add options struct field (row count, colours) — avoid one-off magic numbers in picker.
5. Verify at **1280×720** and **2×** display scale on macOS.
6. Record delta in `FIGMA_UI_AUDIT.md` token table.

---

## Related docs

- `docs/VISUALIZATION_UI_GATE5.md` — original wavetable stack spec
- `docs/OBSIDIAN_BUILD_MAP.md` — Phase 2d per-osc context grids
- `docs/FIGMA_UI_AUDIT.md` — numeric layout tokens · nodes `28:921`–`28:1605`, `37:787`
- `plugin/src/ui/assets/ENGINE_ICONS.md` — 16×16 monoline icons (separate from 80px thumbs)
