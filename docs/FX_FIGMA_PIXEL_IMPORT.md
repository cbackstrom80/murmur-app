# FX Figma Pixel Import — Master Catalog

**File:** [`PFt0LG6XmOiZWcSoUXIWIg`](https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/)  
**Verified:** 2026-08-19 via Figma MCP `get_metadata` + `get_design_context`

This document lists every FX-related sub-screen in Figma, its node ID, pixel anatomy, code target, and import status.

---

## Architecture (3 view layers)

```
Design → FX
├── murmur-fx-card-browser (152:4)     ← landing (CARDS view)
├── murmur-design-fx (35:4)            ← chain editor (CHAIN view)
├── murmur-fx-* × 10                   ← per-effect detail (DETAIL view)
└── murmur-master-quasar-binaural (102:4) ← Quasar hero (overlay)
```

Excluded from card browser: **BYP** (routing only), **VOC** (own DESIGN nav tab).

---

## Frame inventory

| # | Figma frame | Node | Size | Opens from | Code target | Status |
|---|-------------|------|------|------------|-------------|--------|
| 0 | `murmur-fx-card-browser` | `152:4` | 1280×720 | DESIGN → FX default | `DesignFxCardBrowser` + chrome | **DONE** |
| 1 | `murmur-design-fx` | `35:4` | 1280×720 | CARDS ↔ CHAIN toggle | `DesignFxPanel` chain mode | **DONE** |
| 2 | `murmur-fx-saturation` | `63:8` | 1280×720 | SAT card / chain chip | `DesignFxHeroViz` chip 1 | PARTIAL — unity ref, model badge, drive readout |
| 3 | `murmur-fx-chorus` | `63:368` | 1280×720 | CHR card | chip 2 | PARTIAL — 3-voice traces, L/R, rate/depth |
| 4 | `murmur-fx-tape` | `63:724` | 1280×720 | TAPE card | chip 3 | PARTIAL — wow/flutter overlay + readouts |
| 5 | `murmur-fx-mood` | `63:1090` | 1280×720 | MOOD card | chip 4 | PARTIAL — mode badge, FREQ/GAIN axes |
| 6 | `murmur-fx-freqshift` | `63:1451` | 1280×720 | FSHF card | chip 5 | PARTIAL — echo rings + shift/FB readout |
| 7 | `murmur-fx-fractal` | `63:1836` | 1280×720 | FRAC card | chip 6 | PARTIAL — grain/scatter readout |
| 8 | `murmur-fx-reverb` | `63:2227` | 1280×720 | REV card | chip 7 | PARTIAL — glow particles + mode badge |
| 9 | `murmur-fx-equalizer` | `63:2590` | 1280×720 | EQ card | chip 8 | PARTIAL |
| 10 | `murmur-fx-compressor` | `63:2934` | 1280×720 | COMP/LIM card | chip 9 | PARTIAL — IN/GR meters |
| 11 | `murmur-fx-limiter` | `63:3309` | 1280×720 | LIM mode in COMP card | chip 10 | PARTIAL |
| 12 | `murmur-master-quasar-binaural` | `102:4` | 1280×720 | QUASAR card | `MasterQuasarPanel` | PARTIAL |

**Hero viz:** `DesignFxHeroViz` — CPU painters only (`VisualPreviewCache`, `FxAnimationAtlas`, live meters). No OpenGL / `MurmurVisualizerComponent` path.

---

## Shared vertical budget (1280×720 content frames)

All `murmur-fx-*` detail frames share this stack (chrome owned by `MurmurChromeBar` in app):

| Region | Figma layer | Height | Code constant |
|--------|-------------|--------|---------------|
| Outer margin | frame padding | 16px sides | `kDesignFxPageSectionGap` |
| Chrome | `header-bar` | 60px | `kChromeBarHeightDesign` |
| Signal chain | `fx-signal-chain-container` | 119px | `kDesignFxPageSignalChainSectionHeight` |
| Gap | — | 12px | `kDesignFxPageSectionGap` |
| Detail workspace | `middle-workspace` | 419px | flex |
| Detail panel | `selected-fx-detail-panel` | 360px | `kDesignFxPageDetailFullWidth` × body |
| Routing | `vst-bottom-bar` | 54px | `kDesignFxPageRoutingBarHeight` |

**Card browser (`152:4`) differs:** no signal chain; adds `sub-header-bar` (30px) + `grid-container` + `status-bar` (46px).

---

## Card browser (`152:4`) — pixel spec

| Element | Figma | Code |
|---------|-------|------|
| Frame | 1280×720 | Design FX page bounds |
| Sub-header | 30px, title 18px + subtitle 10px | `kDesignFxCardBrowserSubHeaderHeight` |
| Sub-header title | "FX MODULES" 11px | `modulesLabel_` |
| Sub-header hint | "Click any card to open its full editor" 10px | `modulesHintLabel_` |
| Toggle row | 100×26, pills 44×18 | `cardsToggle_` / `chainToggle_` |
| Grid | 2 rows × 5 cols, gap 12px | `kDesignFxCardGap` |
| Card | 230×280, pad 12, radius 8, border 1.5px `#3e4554`, fill `#161822` | `DesignFxCardBrowser` |
| Accent strip | 206×3, top of card | per-card accent color |
| Title row | 11px title + 7px category right | Figma `152:55` |
| Blurb | 8px `#6b7280` | Figma copy per card |
| Mini viz | 200×50 | `paintCardVis()` |
| Mini knobs | 3× 48px col, 24px dial, 6px label, 7px value | `paintCardKnobs()` |
| Divider | 1px line above footer | |
| Footer | "CLICK TO EDIT ↗" 7px + 22×14 ON toggle | |
| Status bar center | MOD SOURCES + LFO1/2 ENV1/2 chips | `paintCardBrowserModChips()` |
| Status bar right | FX LOAD + PANIC RESET | `statusPanicButton_` |

### Card → detail routing

| Card (152:*) | Opens |
|--------------|-------|
| `fx-card-saturator` | `63:8` |
| `fx-card-chorus` | `63:368` |
| `fx-card-tape` | `63:724` |
| `fx-card-mood filter` | `63:1090` |
| `fx-card-freq shift` | `63:1451` |
| `fx-card-fracture` | `63:1836` |
| `fx-card-reverb` | `63:2227` |
| `fx-card-eq` | `63:2590` |
| `fx-card-comp-lim` | `63:2934` |
| `fx-card-quasar` | `102:4` |

---

## Detail panel (`63:*`) — pixel spec

Inside `selected-fx-detail-panel` (1248×360, pad 16):

| Element | Figma | Code |
|---------|-------|------|
| Focused header | 18px | `paintFocusedHeader()` |
| LED ring | 14×14 | active dot |
| Title | 11px bold + slot label | `selectedChipTitle()` |
| Preset dropdown | 142–165×18 | `presetChipBounds_` |
| Status line | right side MODEL/ENGINE text | hero spec |
| ACTIVE toggle | 48×16 | `activeToggleBounds_` |
| Knob grid | 280×174, 2×3 @ 64×58, 32px dials, 44px col gap | `FxChainStrip` design mode |
| Mode strip | 280×26, pills 22px tall | `detailStrip_` |
| Hero viz | 448×296 (std) or 1082×220 (EQ) | `DesignFxHeroViz` |

### EQ unique layout (`63:2590`)

- Graph-first: `main-eq-graph-area` 1082×296
- Sidebar: `eq-side-sidebar` 120×174 (OUT GAIN, ANALYZER toggle, MIX)
- Band value chips under graph (parameter-strip 1082×29)

---

## Quasar (`102:4`) — pixel spec

| Region | Size | Code |
|--------|------|------|
| Header | 44px (breadcrumb chain) | `QuasarChainHeader` |
| Binaural viz | 1248×318 | `QuasarBinauralFieldView` |
| Primary knobs | 1248×105, 7× 80px cells, 44px dials | `QuasarPrimaryKnobRow` |
| Engine + spatial cards | 618×141 each | `QuasarEngineCard`, `QuasarSpatialCard` |
| Telemetry bar | 32px | `QuasarTelemetryBar` |

---

## Import sprint order

1. **Card browser pixel pass** (`152:4`) — mini viz, knobs, typography, footer ← *done*
2. **Sub-header + toggle** (`152:41`) — two-line title block ← *done*
3. **Detail header** — preset chevron, MODEL/ENGINE status line per chip ← *done*
4. **Per-hero viz tuning** — CPU-only `DesignFxHeroViz` painters vs Figma `63:*` (no GL) ← *in progress*
5. **EQ layout** — 1082px graph + 120px sidebar + param strip ← *done*
6. **Quasar** — breadcrumb header, 44px primary knobs, bottom card heights ← *done*
7. **Code Connect stubs** — one `.figma.ts` per `murmur-fx-*` frame ← *DesignFxHeroViz.figma.ts started*

---

## MCP fetch cheat sheet

```
get_design_context  fileKey=PFt0LG6XmOiZWcSoUXIWIg  nodeId=152:4   skillNames=figma-design-to-code
get_design_context  fileKey=PFt0LG6XmOiZWcSoUXIWIg  nodeId=63:8
get_design_context  fileKey=PFt0LG6XmOiZWcSoUXIWIg  nodeId=63:2590
get_design_context  fileKey=PFt0LG6XmOiZWcSoUXIWIg  nodeId=102:4
get_metadata        fileKey=PFt0LG6XmOiZWcSoUXIWIg  nodeId=63:8    # layer tree only
```
