# QUASAR — Figma Build Guide (Standalone Spatial Plugin)

**Audience:** Figma designers and prototype builders  
**Implementation truth:** native JUCE/C++ in `quasar_plugin/` (not React/WebView)  
**Cross-reference:** product spec [`QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md) · MURMUR token source [`OBSIDIAN_BUILD_MAP.md`](OBSIDIAN_BUILD_MAP.md) · pixel audit [`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md)  
**Audit date:** 2026-08-17

---

## Product context

**QUASAR** is a **standalone headphone-first binaural spatial effect** (VST3 / AU / Standalone). It is **not** part of MURMUR's 8-engine synth UI.

| Property | Value |
|----------|-------|
| Product | QUASAR |
| Bundle ID | `com.patchwork.quasar` |
| Core DSP | `pw8::effects::BinauralSpaceProcessor` |
| Signal metaphor | **L input → QSR1 (cyan)** · **R input → QSR2 (violet)** · **CNTR dry anchor** |
| Stereo split | Always on in standalone — each side of a stereo field is placed independently in headphone space |
| Companion product | MURMUR synth — load matching `.quasar` after MURMUR for full Interstellar Spatial scenes |

QUASAR was extracted from MURMUR's removed GLOBAL → QUASAR tab. Reuse Obsidian visual language, but treat QUASAR as its **own single-screen plugin** — no COMPACT / PLAY / DESIGN mode tabs, no engine grid, no mod matrix (post-MVP).

---

## Figma file status

| Item | Status |
|------|--------|
| MURMUR Obsidian file `PFt0LG6XmOiZWcSoUXIWIg` | **No Quasar / spatial / binaural frames found** (searched 2026-08-17) |
| Legacy MURMUR GLOBAL QUASAR tab | **Removed from product** — ASCII wireframe in [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) §4.1 is historical reference only |
| `figma-connect` stubs for Quasar | **NOT STARTED** — no `Quasar*.figma.ts` in repo |
| Reusable MURMUR atoms | **Reuse:** `glow-ring-knobs` (`21:4`), pill/chip patterns from `MetadataFacetRow`-style facet rows, Obsidian palette variables |

**Recommendation:** Add a dedicated Figma page **`quasar-standalone`** inside the MURMUR Obsidian file (shared tokens) or a sibling file keyed to the same variable collection.

---

## Target frame sizes

| Frame | Size | Role |
|-------|------|------|
| **`quasar-play-default`** *(primary)* | **920 × 720** | Matches `QuasarEditor` default — **build this first** |
| **`quasar-play-deep-open`** | **920 × 720** | Same width; DEEP panel expanded (adds ~318px of knob real estate at bottom) |
| **`quasar-play-hero`** *(optional polish)* | **1280 × 720** | Marketing / future layout — letterbox or expand wireframe; not current C++ default |
| Resize bounds (code) | **720 × 520** min · **1400 × 900** max | Prototype should not spec below min |

```cpp
// quasar_plugin/src/QuasarEditor.cpp
setResizeLimits(720, 520, 1400, 900);
setSize(920, 720);
```

**Designer note:** At 920×720 with DEEP closed, ~318px of bottom area is **empty in code today**. Figma should spec how to use that space (preset strip, CNTR meter, orbit motion preview) even if C++ has not caught up yet — mark those regions `DESIGN-ONLY` in handoff.

---

## Tech stack reality

| Figma assumption | Actual codebase |
|------------------|-----------------|
| Same 1280×720 as MURMUR PLAY | **920×720 default** — narrower standalone effect window |
| GLOBAL sub-tab inside MURMUR | **Separate plugin editor** — `QuasarEditor` only |
| Spherical scope (historical spec) | **Pseudo-3D headphone wireframe** — `QuasarSpatialWireframeView` (top-down floor grid + head ellipse + L/R markers) |
| Preset browser in header | **NOT STARTED** — presets load via host/JSON; no UI browser yet |
| Mod matrix / LFO orbit | **Roadmap** — macros ORBIT/SPREAD only for MVP |

**Status legend:** `DONE` · `PARTIAL` · `NOT STARTED` · `DESIGN-ONLY`

---

## Phase completion estimate

| Phase | Scope | Est. complete |
|-------|--------|---------------|
| **1** — Tokens & Quasar accents | Reuse Obsidian + spatial role colors | **~80%** (palette exists in code) |
| **2** — Primitives | Wireframe, glow knobs, facet chips, preset bar | **~40%** |
| **3** — Full-screen layout | PLAY + utility + DEEP | **~55%** |
| **4** — Interaction & preset states | Drag markers, macro sweeps, 20 play presets | **~25%** |
| **Overall** | | **~45%** |

---

## PHASE 1 — Design tokens & Quasar accents

Extract / mirror Obsidian variables from MURMUR Phase 1 ([`OBSIDIAN_BUILD_MAP.md`](OBSIDIAN_BUILD_MAP.md)). QUASAR code already implements the tri-accent system in `quasar_plugin/src/ui/theme/ObsidianPalette.h`.

### Core structure tokens

| Token role | Code value | Figma variable suggestion | Use on QUASAR |
|------------|------------|---------------------------|---------------|
| Background top | `#0B0C0F` | `bg/deep-top` | Root gradient top |
| Background bottom | `#101216` | `bg/deep-bottom` | Root gradient bottom |
| Panel / recessed | `#16181D` | `panel/base` | Wireframe well, knob text boxes |
| Panel raised | `#1C1F26` | `panel/raised` | Chip inactive fill |
| Border | `#232630` | `border/default` | Wireframe grid lines |
| Border bright | `#30343F` | `border/bright` | Head wireframe stroke |
| Text primary | `#ECEEF2` | `text/primary` | Knob labels |
| Text dim | `#858B98` | `text/dim` | Help copy, scope subtitle |

### Quasar-specific accent roles *(do not swap)*

| Role | Code | Figma | UI element |
|------|------|-------|------------|
| **QSR1 / L feed** | `#7FE7E0` (cyan) | `accent/qsr1` | L ORBIT · L DEPTH · L LIFT knobs; L FEED marker + tether |
| **QSR2 / R feed** | `#B9A8FF` / `#8C79E8` (violet) | `accent/qsr2` / `accent/qsr2-deep` | R ORBIT · R DEPTH · R LIFT; R FEED marker |
| **Macro KOINS** | `#E8A33D` (amber) | `accent/macro` | ORBIT macro knob |
| **Macro SPREAD** | violet deep | `accent/spread` | SPREAD macro knob (code uses `kMurmurVioletDeep`) |
| **Performance legend** | amber | `accent/warm` | "PLAY · 2 macro KOINS…" line |

Optional Figma-only spatial teal (`#00FFD0` from MURMUR build map) may be used for **glow emphasis** on wireframe tethers — code uses muted cyan today.

### Typography & spacing

| Spec | Code today | Figma target |
|------|------------|--------------|
| Title | Avenir Next 22px, accent | **QUASAR** wordmark, centred-left |
| Help | 10px dim | 2 lines max, 40px row budget |
| Knob name | 11px label, uppercase | Below dial |
| Scope caption | 10px dim | "HEADPHONE SPACE · drag markers" |
| Outer padding | **12px** all sides | Frame inset |
| Section gaps | 4 / 6px | Between header rows |
| Knob max diameter | **72px** | `GlowKnob` property |
| Wireframe corner radius | **10px** | Recessed panel |

**Phase 1 status:** **PARTIAL** — colors/fonts exist in C++; Figma variables not yet forked for Quasar page.

---

## PHASE 2 — Shared primitives (bottom-up)

Build these components before the full screen.

### 2a. `quasar-spatial-wireframe` → `QuasarSpatialWireframeView`

**C++ owner:** `quasar_plugin/src/ui/components/QuasarSpatialWireframeView.{h,cpp}`  
**Status:** **PARTIAL** — draggable L/R markers wired; no CNTR dot; no LFO motion

| Sub-layer | Spec | Interaction |
|-----------|------|-------------|
| Recessed panel | 10px radius, `panel/base` fill | — |
| Title strip | 18px top band, left-aligned caption | Static |
| Floor grid | 5 horizontal + 6 radial lines, perspective inset | Decorative |
| Head wireframe | 36×44 ellipse + 60×18 headband arc at ~58% height | Listener origin |
| L FEED marker | Cyan glow dot 7.5px (9px dragging) + curved tether | **Draggable** → `qsr1Angle`, `qsr1Distance`, `qsr1Height` |
| R FEED marker | Violet glow dot + tether | **Draggable** → `qsr2Angle`, `qsr2Distance`, `qsr2Height` |
| Hit radius | 18px | Priority to nearest marker |

**Variant states to prototype:**

| Variant | L marker | R marker | Notes |
|---------|----------|----------|-------|
| `default` | front-left | front-right | Preset 001 WIDE STEREO SPLIT |
| `drag-l` | enlarged + bright tether | default | — |
| `drag-r` | default | enlarged | — |
| `rear` | behind head | behind head | Preset 012 BEHIND HEAD |
| `halo` | elevated | elevated | Preset 005 OVERHEAD HALO |
| `cross` | diagonal front-high | diagonal rear-low | Preset 014 DIAGONAL CROSS |

Historical MURMUR spec (spherical scope + CNTR centre dot + ear profile) → mark **`DESIGN-ONLY`** until Phase 4+.

---

### 2b. `glow-knob-quasar` → `GlowKnob`

**C++ owner:** `quasar_plugin/src/ui/components/GlowKnob.{h,cpp}`  
**Reuse Figma:** MURMUR `glow-ring-knobs` (`21:4`) with per-instance accent override

| Property | Value |
|----------|-------|
| Style | Rotary horizontal/vertical drag |
| Text box | Below, 76×16, raised panel fill |
| Name label | 14px band below text box, centred uppercase |
| Default accent | Violet (deep controls); overridden per knob |

**Accent variants:** `macro-orbit` (amber) · `macro-spread` (violet deep) · `spatial-l` (cyan) · `spatial-r` (violet) · `neutral` (default violet)

---

### 2c. `metadata-facet-row` → `MetadataFacetRow`

**C++ owner:** `quasar_plugin/src/ui/components/MetadataFacetRow.{h,cpp}`  
**Pattern:** Row label + horizontal chip strip (scroll if needed)

| Instance | Label | Chips | APVTS param |
|----------|-------|-------|-------------|
| Sidechain mode | `SC AUX` | `QSR2` · `SUM` | `sidechainToQsr2` |
| Delay sync | `DLY SYNC` | `FREE` · `TEMPO` | `quasarDelaySync` |
| Delay division | `DLY DIV` | `1/32` … `8/1` (9 chips) | `quasarDelaySyncDivision` — **visible only when TEMPO** |

Chip active: cyan tint ~42% on `accent/qsr1`; inactive: `panel/raised`.

---

### 2d. `deep-toggle` → `juce::TextButton`

| State | Label | Width |
|-------|-------|-------|
| Collapsed | `DEEP ▾` | 72px |
| Expanded | `DEEP ▴` | 72px |

---

### 2e. Preset bar *(DESIGN-ONLY for MVP)*

**C++ owner:** **NOT STARTED** (roadmap item 4 in product spec)

Spec for Figma now so engineering can implement later:

| Element | Spec |
|---------|------|
| Preset name | Centred or left, chevrons prev/next |
| Bank facet | `PLAY` · `INTERSTELLAR` · `USER` |
| File extension | `.quasar` |
| Default showcase | `001-wide-stereo-split` |

---

**Phase 2 status:** **PARTIAL** — wireframe + knobs + facet rows exist; preset bar and figma-connect stubs do not.

---

## PHASE 3 — Full-screen layout assembly

Build two frames: **`quasar-play-default`** (DEEP closed) and **`quasar-play-deep-open`**.

### Region tree (920 × 720, DEEP closed)

Pixel budgets derived from `QuasarEditor::resized()` — content width **896px** (920 − 24 padding).

```
quasar-play-default (920×720)
├── root-padding (12px inset → 896×696 content)
│   ├── header-title                    896 × 28    QuasarEditor · titleLabel_     "QUASAR"
│   ├── gap                             896 × 4
│   ├── header-help                     896 × 40    QuasarEditor · helpLabel_      stereo-split explainer
│   ├── gap                             896 × 4
│   ├── header-play-legend              896 × 16    QuasarEditor · playLegendLabel_  amber PLAY line
│   ├── gap                             896 × 6
│   ├── main-play-row                   896 × 220
│   │   ├── spatial-wireframe           ~597 × 220  QuasarSpatialWireframeView     2/3 width
│   │   └── play-knob-column            ~299 × 220
│   │       ├── macro-koin-row          299 × 88    2×1 grid: ORBIT · SPREAD
│   │       ├── gap                     299 × 4
│   │       └── spatial-knob-grid       299 × 128   2×3 grid: L/R ORBIT·DEPTH·LIFT
│   ├── gap                             896 × 6
│   ├── sidechain-caption               896 × 16    sidechainLabel_
│   ├── gap                             896 × 4
│   ├── utility-row                     896 × 34
│   │   ├── deep-toggle                 72 × 34
│   │   ├── sc-aux-facet                150 × 34    MetadataFacetRow SC AUX
│   │   ├── dly-sync-facet              160 × 34    MetadataFacetRow DLY SYNC
│   │   └── dly-div-facet               260 × 34    (conditional on TEMPO)
│   └── bottom-reserved                 896 × ~318  EMPTY in code — design opportunity
```

### Region tree (DEEP open)

Replace `bottom-reserved` with:

```
│   ├── gap                             896 × 8
│   └── deep-knob-grid                  896 × ~310  6 columns × 3 rows (17 knobs)
│       Row 1: MIX · CNTR · QSR1 LVL · QSR2 LVL · L ROOM · R ROOM
│       Row 2: L SIZE · R SIZE · DLY TIME · DLY FDBK · DLY VOL · OUTPUT
│       Row 3: X-FEED · QSR HPF · CNTR HPF · L DAMP · R DAMP · (empty)
```

Row height in code: `min(92, available / rows)`.

### Region → C++ component map

| Figma region | C++ component | File |
|--------------|---------------|------|
| Root / background gradient | `QuasarEditor` | `QuasarEditor.cpp` |
| Title + help + legends | `juce::Label` ×3 | `QuasarEditor.cpp` |
| Spatial wireframe | `QuasarSpatialWireframeView` | `ui/components/QuasarSpatialWireframeView.*` |
| Macro knobs | `GlowKnob` ×2 | `orbitMacro`, `spreadMacro` |
| Spatial knobs | `GlowKnob` ×6 | QSR1/2 angle, distance, height |
| Sidechain caption | `juce::Label` | `sidechainLabel_` |
| DEEP toggle | `juce::TextButton` | `deepToggle_` |
| SC / DLY facets | `MetadataFacetRow` ×3 | `MetadataFacetRow.*` |
| DEEP knobs | `GlowKnob` ×17 | `rebuildDeepKnobs()` |
| Look & feel | `ObsidianLookAndFeel` | `ui/theme/ObsidianLookAndFeel.*` |

### Optional hero layout (1280 × 720) — `DESIGN-ONLY`

If building the wider frame for visual polish:

| Change vs 920 | Rationale |
|---------------|-----------|
| Expand wireframe to ~780px width | Hero spatial canvas |
| Move macro knobs below wireframe as horizontal strip | Balanced 16:9 |
| Add preset bar 44px top | Matches MURMUR chrome height language |
| Fill bottom with DECK meters or orbit trail | Uses dead space |

Tag hero deltas explicitly in handoff — do not assume 1280 is implemented.

**Phase 3 status:** **PARTIAL**

---

## PHASE 4 — Interaction states & preset reference

### Macro KOINS behavior (prototype wiring)

Document in Figma prototype annotations — macros fan at DSP via `QuasarSpatialMacros.hpp`:

| Macro | Param | Neutral | CCW (< 0.5) | CW (> 0.5) |
|-------|-------|---------|-------------|------------|
| **ORBIT** | `orbitMacro` | 0.5 | Rotate L+R feeds counter-clockwise (±180° total) | Rotate clockwise |
| **SPREAD** | `spreadMacro` | 0.5 | Narrow angle separation + pull depth/height in | Widen separation + push depth/height out |

Prototype suggestion: link ORBIT slider to **rotate both markers** around head centre; link SPREAD to **move markers apart/converge**.

### Spatial knob ↔ wireframe sync

All six PLAY knobs and wireframe drag targets write the same APVTS fields — prototype should keep markers and knobs in sync.

| Control | Param | Range | Display |
|---------|-------|-------|---------|
| L ORBIT | `qsr1Angle` | 0–360° | Integer + ° |
| L DEPTH | `qsr1Distance` | 0–1 | 2 decimals |
| L LIFT | `qsr1Height` | −1–1 | 2 decimals |
| R ORBIT | `qsr2Angle` | 0–360° | Integer + ° |
| R DEPTH | `qsr2Distance` | 0–1 | 2 decimals |
| R LIFT | `qsr2Height` | −1–1 | 2 decimals |

### Discrete facet prototypes

| Facet | Values | Param |
|-------|--------|-------|
| SC AUX | QSR2 (default) / SUM | `sidechainToQsr2` |
| DLY SYNC | FREE / TEMPO | `quasarDelaySync` |
| DLY DIV | 1/32 … 8/1 | `quasarDelaySyncDivision` |
| OUTPUT *(DEEP knob)* | PHONE / SPEAKER / AUTO | `quasarOutputMode` |

### PLAY preset reference (20 factory `.quasar`)

Use these as **layout/content fixtures** when building wireframe variants and demo prototypes. Full param payloads live in `content/presets/quasar/play/`.

| # | File | Display name | Design intent | Key spatial pose |
|---|------|--------------|---------------|------------------|
| 001 | `001-wide-stereo-split.quasar` | WIDE STEREO SPLIT | **Default** — neutral macros, classic L/R split | L 72° / R 288°, spread 0.62 |
| 002 | `002-tight-center-anchor.quasar` | TIGHT CENTER ANCHOR | Strong CNTR, narrow spread | spread 0.18, close distances |
| 003 | `003-left-intimate.quasar` | LEFT INTIMATE | Asymmetric — L close, R far | L dist 0.18 / R dist 0.72 |
| 004 | `004-right-intimate.quasar` | RIGHT INTIMATE | Mirror of 003 | R dist 0.16 / L dist 0.68 |
| 005 | `005-overhead-halo.quasar` | OVERHEAD HALO | Both feeds elevated | height ±0.7 |
| 006 | `006-floor-pressure.quasar` | FLOOR PRESSURE | Low elevation, bass-safe HPF | height −0.5 |
| 007 | `007-cathedral-wash.quasar` | CATHEDRAL WASH | Large room + long delay | room 0.6+, delay 680ms |
| 008 | `008-closet-dry.quasar` | CLOSET DRY | Tiny room, short distance | room ~0.1, dist ~0.14 |
| 009 | `009-orbit-ready.quasar` | ORBIT READY | Orbit macro biased 0.72 | sweep ORBIT knob in prototype |
| 010 | `010-spread-maximum.quasar` | SPREAD MAXIMUM | spread 0.88 — wide separation | extreme L/R angles |
| 011 | `011-narrow-beam.quasar` | NARROW BEAM | spread 0.12 — collapsed field | angles near 0°/360° |
| 012 | `012-behind-head.quasar` | BEHIND HEAD | Rear orbit | angles 145° / 215° |
| 013 | `013-front-stage.quasar` | FRONT STAGE | Club PA in front | angles 335° / 25° |
| 014 | `014-diagonal-cross.quasar` | DIAGONAL CROSS | X-shaped height/distance | extreme height contrast |
| 015 | `015-tempo-echo-orbit.quasar` | TEMPO ECHO ORBIT | DLY SYNC TEMPO, 1/4 | show DLY DIV row |
| 016 | `016-canyon-tail.quasar` | CANYON TAIL | Long delay, high feedback | delay 1450ms, fdbk 0.78 |
| 017 | `017-dry-panner.quasar` | DRY PANNER | Utility — no room/delay | room 0, delay vol 0 |
| 018 | `018-speaker-glue.quasar` | SPEAKER GLUE | OUTPUT SPEAKER + crossfeed | output mode 1, x-feed 0.35 |
| 019 | `019-sidechain-qsr2.quasar` | SIDECHAIN QSR2 | SC AUX QSR2 preset | sidechainToQsr2 1, hot QSR2 |
| 020 | `020-neon-club.quasar` | NEON CLUB | Bright HPF, tight room, wide | spread 0.78, HPF 220Hz |

Regenerate batch: `python3 scripts/generate_quasar_play_presets.py`

**Recommended Figma preset demo set (5 frames):** 001 default · 009 orbit-ready · 010 spread-max · 012 behind-head · 019 sidechain-qsr2

**Phase 4 status:** **PARTIAL** — presets exist; Figma interaction frames not started.

---

## Parameter map (28 APVTS → UI)

Source: `quasar_plugin/src/QuasarParamLayout.cpp`

### PLAY surface (always visible)

| Param ID | Label (UI) | Control type | Surface |
|----------|------------|--------------|---------|
| `orbitMacro` | ORBIT | GlowKnob (amber) | Macro row |
| `spreadMacro` | SPREAD | GlowKnob (violet deep) | Macro row |
| `qsr1Angle` | L ORBIT | GlowKnob (cyan) | Spatial grid |
| `qsr1Distance` | L DEPTH | GlowKnob (cyan) | Spatial grid |
| `qsr1Height` | L LIFT | GlowKnob (cyan) | Spatial grid |
| `qsr2Angle` | R ORBIT | GlowKnob (violet) | Spatial grid |
| `qsr2Distance` | R DEPTH | GlowKnob (violet deep) | Spatial grid |
| `qsr2Height` | R LIFT | GlowKnob (violet) | Spatial grid |
| `sidechainToQsr2` | SC AUX | MetadataFacetRow | Utility row |

### Utility / conditional

| Param ID | Label | Control | Visible when |
|----------|-------|---------|--------------|
| `quasarDelaySync` | DLY SYNC | MetadataFacetRow | Always |
| `quasarDelaySyncDivision` | DLY DIV | MetadataFacetRow | `quasarDelaySync` = TEMPO |

### DEEP panel (17 knobs, toggle to show)

| Param ID | Label | Range / notes |
|----------|-------|---------------|
| `mix` | MIX | 0–1 |
| `cntrLevel` | CNTR | 0–1 |
| `qsr1Level` | QSR1 LVL | 0–1 |
| `qsr2Level` | QSR2 LVL | 0–1 |
| `qsr1RoomAmount` | L ROOM | 0–1 |
| `qsr2RoomAmount` | R ROOM | 0–1 |
| `qsr1RoomSize` | L SIZE | 0.2–3.0 |
| `qsr2RoomSize` | R SIZE | 0.2–3.0 |
| `qsr1RoomDamping` | L DAMP | 0–1 |
| `qsr2RoomDamping` | R DAMP | 0–1 |
| `quasarDelayTimeMs` | DLY TIME | 3–20000 ms |
| `quasarDelayFeedback` | DLY FDBK | 0–1 |
| `quasarDelayVolume` | DLY VOL | 0–1 |
| `quasarOutputMode` | OUTPUT | 0=PHONE 1=SPEAKER 2=AUTO |
| `quasarCrossfeed` | X-FEED | 0–1 |
| `inputSplitHpfHz` | QSR HPF | 20–500 Hz |
| `cntrHpfHz` | CNTR HPF | 20–300 Hz |

### Wireframe-only (no dedicated knob on PLAY column)

Same six spatial params as spatial grid — written by drag.

---

## Prototype navigation

QUASAR MVP is a **single-page editor** — no MURMUR-style tab router.

| Frame | Prototype entry | Links |
|-------|-----------------|-------|
| `quasar-play-default` | **Start** | Click `DEEP ▾` → `quasar-play-deep-open` |
| `quasar-play-deep-open` | — | Click `DEEP ▴` → default |
| `quasar-play-default` | — | Toggle `DLY SYNC` → TEMPO shows/hides `DLY DIV` row |
| `quasar-play-default` | — | Drag L/R marker → swap to `drag-l` / `drag-r` variants |
| Preset demos | Optional secondary flows | Instant jump between 5 preset fixture frames |

**No multi-page tabs** for MVP. Future overlays (mark `DESIGN-ONLY`):

- Preset browser modal (mirror MURMUR `murmur-preset-browser` `27:6` layout language, simplified)
- Settings / quality tier (Eco/Normal/High HRTF)

---

## Delta vs current C++ UI

| Area | Figma should spec | Code today | Status |
|------|-------------------|------------|--------|
| Frame size | 920×720 primary | 920×720 default | **DONE** |
| Wireframe | Headphone pseudo-3D + draggable L/R | Implemented | **PARTIAL** |
| CNTR anchor dot in scope | Visible centre anchor | Not drawn | **NOT STARTED** |
| Spherical scope / ear profile | From legacy GLOBAL plan | Not implemented | **DESIGN-ONLY** |
| Preset bar / browser | Top chrome | Not implemented | **NOT STARTED** |
| Bottom 318px dead space | Meters, preset grid, or orbit trail | Empty when DEEP closed | **DESIGN-ONLY** |
| DEEP panel organization | Grouped MIX / ROOM / DELAY / OUTPUT sections | Flat 6×3 knob grid | **PARTIAL** |
| Per-path room tabs (QSR1 / QSR2) | Tabbed DEEP room | All knobs visible at once | **NOT STARTED** |
| LFO orbit motion in scope | Animated markers | Static | **DESIGN-ONLY** |
| figma-connect | `QuasarSpatialWireframeView.figma.ts`, etc. | Missing | **NOT STARTED** |
| MURMUR GLOBAL QUASAR tab | N/A — product removed | Removed from MURMUR | — |

---

## Handoff checklist (Figma → Cursor)

Before implementation pass, confirm:

- [ ] **Frame names** match region tree (`quasar-play-default`, `quasar-play-deep-open`)
- [ ] **Primary size** annotated: 920×720 (not only 1280×720)
- [ ] **Node IDs** recorded on each region for `FIGMA_UI_AUDIT.md` entry
- [ ] **Component keys** published: `quasar-spatial-wireframe`, `glow-knob-quasar`, `metadata-facet-row`
- [ ] **Accent overrides** documented per knob instance (cyan vs violet vs amber)
- [ ] **Wireframe variants** for presets 001, 009, 010, 012, 019
- [ ] **Interaction** notes for marker drag hit area (18px) and macro behavior
- [ ] **Conditional UI**: DLY DIV row hidden unless TEMPO; DEEP panel swap
- [ ] **DESIGN-ONLY** layers flagged (preset bar, CNTR dot, hero 1280 layout)
- [ ] **Prototype link** attached in Figma cover / dev resources
- [ ] **Export sizes**: wireframe icons @1x; no bitmap assets required for MVP (vector paint in C++)
- [ ] **Code Connect stubs** planned: `quasar_plugin/src/ui/figma-connect/QuasarSpatialWireframeView.figma.ts`

---

## Build order summary

```
PHASE 1 — TOKENS
  Reuse Obsidian bg/text/border + Quasar accent roles (qsr1 cyan, qsr2 violet, macro amber)

PHASE 2 — PRIMITIVES
  2a. quasar-spatial-wireframe (+ drag + preset variants)
  2b. glow-knob-quasar (accent variants)
  2c. metadata-facet-row (SC AUX, DLY SYNC, DLY DIV)
  2d. deep-toggle
  2e. preset-bar (DESIGN-ONLY)

PHASE 3 — FULL SCREEN
  3a. quasar-play-default (920×720)
  3b. quasar-play-deep-open
  3c. quasar-play-hero (1280×720, optional)

PHASE 4 — INTERACTION & PRESETS
  4a. Macro ORBIT/SPREAD prototype motion
  4b. Five preset fixture frames
  4c. Facet / DEEP toggle flows
```

---

## Related docs

- [`QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md) — product spec, build commands, roadmap
- [`OBSIDIAN_BUILD_MAP.md`](OBSIDIAN_BUILD_MAP.md) — MURMUR Obsidian tokens & shared primitives
- [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) — full DSP architecture + historical GLOBAL tab wireframe
- [`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md) — MURMUR frame inventory (reuse knob atoms from here)
