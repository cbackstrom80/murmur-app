# MURMUR 8-ENGINE — Obsidian Theme Build Map

**Canonical implementation order** for the Obsidian Alt UI (Figma file `PFt0LG6XmOiZWcSoUXIWIg`).  
**Cross-reference:** numeric layout deltas live in [`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md).  
**Audit date:** 2026-08-17

---

## Tech stack reality

| Figma / brief assumption | Actual codebase |
|--------------------------|-----------------|
| React / JUCE WebView | **Native JUCE/C++** — `plugin/src/ui/` |
| Single header component | **Split chrome:** `PatchBrowserBar`, `VstTopBar`, `DesignModeHeaderBar` via `MurmurRootEditor` |
| 12-slot FX chain (35:4) | **7 slots** (3 layer + 4 master) in `FxChainStrip` |
| Unified COMPACT \| PLAY \| DESIGN tabs | **Figma DONE** · **Code DONE** — `MurmurChromeBar` wired; Basic/Advanced toggle via double-click brand |
| Design sub-nav ENGINE \| ARP \| VOCODER \| FX \| MOD | **Figma DONE** · **Code DONE** — sub-nav swaps `DesignModeEditor` pages; BROWSE opens overlay |

**Status legend:** `DONE` · `PARTIAL` · `NOT STARTED` · `DESIGN-ONLY`

---

## Phase completion estimate

| Phase | Scope | Est. complete |
|-------|--------|---------------|
| **1** — Design tokens & primitives | Palette, fonts, radii, spacing | **~70%** |
| **2** — Shared components | Header, knobs, engine card, context grids, FX chip, footer | **~62%** |
| **3** — View assembly | 10 desktop surfaces + Advanced PLAY adjunct | **~79%** |
| **4** — iPad adaptation | 5 touch layouts | **0%** |
| **5** — Theme system | 5 color skins × variants | **0%** (Figma only) |
| **Overall (Phases 1–3 desktop)** | | **~72%** |

---

## PHASE 1 — Design tokens & primitives

Extract Figma variables → `plugin/src/ui/theme/`.

| Token (build map) | Figma value | Code today | C++ owner | Status |
|-------------------|-------------|------------|-----------|--------|
| Background | `#0A1114` | `kBackgroundTop/Bottom` → `kFigmaBgDeep` `#0A1114` | `ObsidianPalette.h` | **DONE** (2026-08-17) |
| Header | `#0A0E12` | `kHeader` → `kFigmaHeader` `#0A0E12` | `ObsidianPalette.h` | **DONE** (2026-08-17) |
| Teal glow accent | `#00FFD0` | `kAccent` → `kFigmaTeal` `#00FFD0` | `ObsidianPalette.h` | **DONE** (2026-08-17) |
| Dim text | `#596166` | `kTextDim` → `kFigmaTextDim` `#596166` | `ObsidianPalette.h` | **DONE** (2026-08-17) |
| White text | `#E6EBED` | `kTextPrimary` → `kFigmaTextPrimary` `#E6EBED` | `ObsidianPalette.h` | **DONE** (2026-08-17) |
| Pill active | `#004033` | `kAccentDim` / `kFigmaPillActive`; toggle pills in `ObsidianLookAndFeel` | `ObsidianLookAndFeel.cpp` | **DONE** (2026-08-17) |
| Pill inactive | `#141A1F` @ 60% | `kFigmaPillInactive` + manual alpha in components | various | **PARTIAL** |
| Typography | Inter Black/Bold/Medium, 7–11px | **Avenir Next** fallback chain, 10–12.5px floors | `ObsidianFonts.h` | **PARTIAL** — no Inter embed (audit gap) |
| Corner radius — compact pills | 3px | 3–4px ad hoc in `ObsidianDraw` / knobs | `ObsidianDraw.h`, knobs | **PARTIAL** |
| Corner radius — standard pills | 4px | 4–8px by context | `PlayModeLayout.h`, cards | **PARTIAL** |
| Corner radius — header bars | 6px | 8px button faces in LAF | `ObsidianLookAndFeel.cpp` | **PARTIAL** |
| Spacing scale | 4/6/8/12/16px | Named constants in `PlayModeLayout.h` (8, 12, 16, 20, 24…) | `PlayModeLayout.h` | **PARTIAL** — 4px/6px not tokenized |

### Phase 1 gaps (actionable)

1. **Inter font:** Figma specifies Inter; code uses Avenir Next via `ObsidianFonts.h`. Embed licensed Inter or add explicit caption tier mapping — not bundled today.
2. **Token file:** Consider `ObsidianTokens.h` mirroring Figma variable names (`bg/deep`, `accent/teal`, `pill/active`) — today colors are semantic roles aliased to `kFigma*` constants.
3. **Glow stroke weights:** Knob arc thicknesses are per-component, not tokenized (`FIGMA_UI_AUDIT` glow-ring-knobs gap).
4. **Pill inactive alpha:** Some components still hand-roll inactive pill fill; consolidate on `kFigmaPillInactive`.

**Phase 1 status:** **PARTIAL** (~70%) — core Figma color tokens aligned; typography + radii/spacing still open.

---

## PHASE 2 — Shared components (bottom-up)

### 2a. Header system (ALL views)

**Spec:** Logo `MURMUR · 8-ENGINE` · view tabs `COMPACT | PLAY | DESIGN` · design sub-nav `ENGINE | ARP | VOCODER | FX | MOD MATRIX` · preset name + bank + master · compact abbreviations at 320px.

| Source frame | Node ID | C++ owner(s) | Figma | Code |
|--------------|---------|--------------|-------|------|
| Unified header-bar | `39:2` / `39:142` / `39:158` | `MurmurChromeBar` | **DONE** | **DONE** — COMPACT/PLAY/DESIGN tabs + design sub-nav; preset chevrons; double-click brand toggles Basic/Advanced PLAY |
| Design header-bar (legacy) | `37:788` | `DesignModeHeaderBar` (deprecated) | **DONE** | superseded by `MurmurChromeBar` |
| Basic PLAY top-bar (legacy) | `36:5` | `PatchBrowserBar` (hidden) | **DONE** | superseded by `MurmurChromeBar` |
| Advanced PLAY chrome (legacy) | `22:3` | `VstTopBar` (unused) | **DONE** | superseded by `MurmurChromeBar` |

**Orchestration:** `MurmurRootEditor.cpp` switches chrome by `EditorMode` + `PlayViewMode`.

**Phase 2a status:** **Figma DONE** / **Code DONE** — unified `MurmurChromeBar` + design sub-nav routing wired (2026-08-17).

---

### 2b. Glow ring knobs

**Spec:** 2-ring and 3-ring variants; value pills; default/hover/active; interaction reference `21:172`.

| Figma | Node | C++ | Status |
|-------|------|-----|--------|
| `glow-ring-knobs` | `21:4` | `GlowKnob`, `GlowRingButton`, `ConcentricGlowKnob`, `TripleGlowKnob` | **PARTIAL** |
| `glow-knob-interaction-concepts` | `21:172` | — | **DESIGN-ONLY** |

| Component | File | Notes |
|-----------|------|-------|
| Single-parameter knob | `GlowKnob.{h,cpp}` | APVTS + mod rings; primary control primitive |
| Dual-ring (inner/outer param) | `ConcentricGlowKnob.{h,cpp}` | Filter cutoff/res, WT bend; used on `EngineCard` |
| Triple-ring | `TripleGlowKnob.{h,cpp}` | Figma UX-09; figma-connect stub exists |
| Ring button (FX slot, ARP) | `GlowRingButton.{h,cpp}` | `FxChainStrip`, `ArpLauncherChip` |
| Default rotary paint | `ObsidianLookAndFeel.cpp` | Violet default arc; glow via `RadialGlowDraw.h` |

**Phase 2b status:** **PARTIAL**

---

### 2c. Engine card

**Spec:** LED + ENGINE 0X + ON/S/M · 9 osc pills · context grid · pitch/filter knobs · ADSR mini · level fader.

| Source | Node | C++ | Status |
|--------|------|-----|--------|
| Design grid cards | `37:830` children | `EngineCard`, `EngineGridPanel` | **PARTIAL** — design layout wired |
| Full card anatomy | `4:38` / `28:4` | `EngineDetailOverlay` | **PARTIAL** — 3-column deep editor, letterboxed 1440×1024, ENG pills wired |
| Play-board stub | `22:44` | `EngineCard::setPlayBoardCompactMode` | **PARTIAL** — Figma 22:44 dial stubs + orange level tail; osc stubs 32×10 |
| Osc states reference | `28:1953`, `31:4` | — | **DESIGN-ONLY** |

| Sub-region | C++ | Status |
|------------|-----|--------|
| Header (LED, title, ON/S/M) | `EngineCard.cpp` | **PARTIAL** — mix enable/mute/solo wired |
| Type strip (9 pills) | `EngineOscillatorPicker.cpp` | **PARTIAL** — 9 pills on engine 0, 8 on others |
| Context visualizer | `EngineOscillatorPicker.cpp` | **PARTIAL** — 80px design-v2 per-type wireframes; **WT/GRN use real wavetable mesh** via `WavetableMeshPaint` (2026-08-17); play-board stubs unchanged |
| Pitch / filter knobs | `EngineCard.cpp`, `ConcentricGlowKnob` | **PARTIAL** — design v2 hides pitch (NSE/EXT) or filter (FM) per engine group |
| ADSR mini | `EngineAdsrMini.{h,cpp}` | **PARTIAL** |
| Level row | `EngineCard.cpp` | **PARTIAL** |

**Phase 2c status:** **PARTIAL**

---

### 2d. Per-oscillator-type context grids (9 variants)

**Spec:** CLS, WT, FM, ADD, PHS, GRN, NSE, RES, EXT — each with type-specific controls per Figma.

| Figma source | Node | Engine types |
|--------------|------|--------------|
| Context grids A | `28:921` | CLS, WT, FM |
| Context grids B | `28:1281` | ADD, PHS, GRN |
| Context grids C | `28:1605` | NSE, RES, EXT |

**Control groups (spec):**

| Group | Engines |
|-------|---------|
| Full (pitch + filter) | CLS, WT, ADD, PHS, GRN, RES |
| No filter | FM |
| No pitch | NSE, EXT |

| Engine | Card mini-grid (`EngineOscillatorPicker`) | Full wireframe (`OscWireframeHost` / labs) | Status |
|--------|---------------------------------------------|---------------------------------------------|--------|
| **CLS** | 4 waveform cells | `ClassicWireframeView` | **PARTIAL** |
| **WT** | WT position cells | `WavetableStackView` (3D mesh) | **PARTIAL** |
| **FM** | Ratio/feedback presets | `FmWireframeView` | **PARTIAL** — design-v2 thumb adds live mod/carrier wave layers behind OP boxes |
| **ADD** | Partial-count presets | `AdditiveWireframeView` | **PARTIAL** |
| **PHS** | Phase-shape presets | `PhaseShapeWireframeView` | **PARTIAL** |
| **GRN** | Shared WT view + overlay | `WavetableStackView` + granular overlay | **PARTIAL** — grain-cloud viz with centre glow + variable grain dots |
| **NSE** | Noise variant cells | `NoiseWireframeView` | **PARTIAL** |
| **RES** | Mode-count presets | `ResonatorWireframeView` | **PARTIAL** — resonator peak envelope + filled spikes |
| **EXT** | Input selector pills + env follower viz | — | **PARTIAL** — `ExternalInputSource` APVTS (0–3) wired to sub-picker pills; engine passthrough stub |

| **Phase 2d status:** **PARTIAL** (~9/9 engine previews wired in design-v2 80px visualizer; frame strip + harmonic editor + legacy WT/GRN grid now use real table data)

**Graphics plan:** Per-OSC JUCE-themed context thumbs (279×80, procedural paint, no PNG atlas) — see [`UI_GRAPHICS_PLAN.md` — Per-OSC-type context thumb assets](UI_GRAPHICS_PLAN.md#per-osc-type-context-thumb-assets-engine-card). Next: Figma screenshot acceptance (`28:921`–`28:1605`), then extract `EngineOscContextThumb` component.

**FX pre/post routing note:** `fxRoutingPrePost` APVTS param + toggle in `DesignFxPanel` (`35:4`) and PRE/POST tap label in PLAY `FxChainFlowView` select insert-chain order vs master-gain (`Engine::render`: PRE = inserts then gain; POST = layer-B dry merge → gain → inserts on combined bus). Send A/B parallel returns tap pre/post insert chain via `setFxSendLevels()`. Stack-mode layer B uses its own insert chain in PRE only; POST routes merged bus through layer-A insert chain (documented MVP).

---

### 2e. FX slot chip

**Spec:** 12 types (BYP SAT CHR TAPE MOOD FSHF FRAC REV EQ COMP LIM VOC); bypassed / active / selected.

| Source | Node | C++ | Status |
|--------|------|-----|--------|
| Signal chain | `35:4` | `FxChainStrip`, `wireframe/FxChainFlowView` | **PARTIAL** — 7 slots; PLAY dashboard chips show mix bar + selected glow |
| Sprite sheet | `17:4` | — | **DESIGN-ONLY** |

**Phase 2e status:** **PARTIAL**

---

### 2f. Status / footer bar

**Spec:** CPU %, voice count, MIDI in, mod sources, panic reset.

| Source | Node | C++ | Status |
|--------|------|-----|--------|
| Design status-bar | `37:1507` | `VstBottomBar` (`setDesignModeV2Layout`) | **PARTIAL** — 40px; MOD chips painted; CPU hardcoded |
| Play-board status-bar | `22:414` | `VstBottomBar` (`setPlayBoardMode`) | **PARTIAL** — 28px; VOCODER/LFO lab chips |
| Basic PLAY bottom | `36:226` | `VstBottomBar` (desktop play) | **PARTIAL** — engine LEDs, latch/panic painted; latch not wired |

**Phase 2f status:** **PARTIAL**

---

## PHASE 3 — View assembly (top-down)

### 3a. COMPACT VIEW — `4:1134` (320×560)

| Region | C++ | Status |
|--------|-----|--------|
| Full compact shell | `CompactModeEditor` via `PlayModeEditor` | **PARTIAL** — 320×560; asymmetric insets 14/30; chrome 28px; see `murmur-compact-view.4-1134.layout.json` |
| Scope | `CircularScopeView` | **PARTIAL** |
| Performance macros | `PatchFocusPanel` | **PARTIAL** |
| Footer | inline compact chrome | **PARTIAL** |

**Priority (spec):** HIGH · **Status:** **PARTIAL**

---

### 3b. PLAY VIEW (Basic) — `36:4` (1280×720)

| Region | C++ | Status |
|--------|-----|--------|
| Shell | `PlayModeEditor` + `MurmurRootEditor` | **PARTIAL** |
| Top-bar `36:5` | `PatchBrowserBar` desktop chrome | **PARTIAL** — layout constants aligned 2026-08-17 |
| Oscilloscope `36:35` | `OscilloscopeView` | **PARTIAL** — waveform path live; bar viz decorative |
| Macro deck `36:155` | `PatchFocusPanel` | **PARTIAL** |
| Bottom bar `36:226` | `VstBottomBar` | **PARTIAL** |

**Priority:** HIGH · **Status:** **PARTIAL**

---

### 3b-adj. PLAY VIEW (Advanced) — `22:2` `obsidian-play-board`

*Not in original phase list but implemented in code as Advanced PLAY default.*

| Region | C++ | Status |
|--------|-----|--------|
| Full board | `PlayModeEditor` Advanced | **PARTIAL** |
| Chrome `22:3` | `PatchBrowserBar` + `VstTopBar` | **PARTIAL** |
| Engine grid `22:37` | `EngineGridPanel` + compact `EngineCard` | **PARTIAL** |
| Dashboard `22:188` | `DashboardStrip` | **PARTIAL** — live FX/FLT peak meters @ 10Hz via `ScopeVuMeter` + audio tap |
| Status `22:414` | `VstBottomBar` | **PARTIAL** |

**Status:** **PARTIAL**

---

### 3c. DESIGN → ENGINE — `37:787` (1280×1048)

| Region | C++ | Status |
|--------|-----|--------|
| Shell | `DesignModeEditor` + `MurmurRootEditor` | **PARTIAL** — page 1 only |
| Master envelope | `MasterEnvelopePanel` (`82:4`) | **PARTIAL** — ADSR hero above grid; see [`FIGMA_UI_AUDIT.md` Part 6.4](FIGMA_UI_AUDIT.md#64-master-envelope-sub-panels) |
| Header `39:2` | `MurmurChromeBar` | **Figma DONE** / **Code DONE** |
| Grid `37:830` | `EngineGridPanel` (`setDesignModeV2Layout`) | **PARTIAL** |
| Status `37:1507` | `VstBottomBar` | **PARTIAL** |
| Deep editor (overlay) | `EngineDetailOverlay` (`28:4` ref) | **PARTIAL** |

**Priority:** CRITICAL · **Status:** **PARTIAL**

---

### 3d. DESIGN → ARP — `4:1267`

| C++ | Status |
|-----|--------|
| `ArpPanelOverlay`, `ArpStepStrip` | **Figma DONE** / **Code PARTIAL** — full-screen overlay from PLAY; not yet design sub-nav page in C++ |

**Priority:** MEDIUM · **Status:** **PARTIAL**

---

### 3e. DESIGN → VOCODER — `15:4`

| C++ | Status |
|-----|--------|
| `VocoderLabPanel` | **PARTIAL** — overlay; signal diagram zone fixed 110px |

**Priority:** MEDIUM · **Status:** **PARTIAL**

---

### 3f. DESIGN → FX — `35:4`

| C++ | Status |
|-----|--------|
| `DesignFxPanel`, `DesignFxSignalChain`, `FxChainStrip`, `DesignFxHeroViz`, `DesignFxPresetLibrary` | **DONE** — 12-slot rack, full-width per-FX hero editors (Part 2j), preset library, drag-reorder → `fxProcessOrder` engine permute, send A/B parallel returns, live FX load meter |

**Priority:** HIGH · **Status:** **DONE** (2026-08-17 gap closure)

---

### 3g. DESIGN → MOD MATRIX — `27:265`

| C++ | Status |
|-----|--------|
| `ModRoutingOverlay`, `ModRoutingUi`, `ModSourcePalette` | **PARTIAL** — overlay from PLAY footer / LFO lab CTA |

**Priority:** MEDIUM · **Status:** **PARTIAL**

---

### 3h. DESIGN → PRESET BROWSER — `27:6` / modal `74:959`

| C++ | Status |
|-----|--------|
| `PresetBrowserOverlay`, `PatchBrowserBar` | **PARTIAL** — full-page `27:6` (240/696/300) + centered modal `74:959` (`murmur-preset-explorer-overlay`, 1040×620); registry in [`FIGMA_UI_AUDIT.md` Part 6.1](FIGMA_UI_AUDIT.md#61-core-views-canonical-registry) |
| `PlayModeEditor` Basic (`86:4`) | **PARTIAL** — `murmur-basic-view`: envelope hero + portamento + 4 macros; stub `MurmurBasicView.figma.ts` |

**Priority:** LOW · **Status:** **PARTIAL**

---

### 3i. DESIGN → WAVETABLE EDITOR — `27:709`

| C++ | Status |
|-----|--------|
| `WavetableLabPanel`, `WavetableStackView` | **PARTIAL** — overlay from PLAY labs |

**Priority:** LOW · **Status:** **PARTIAL**

---

### 3j. DESIGN → DUAL LFO — `15:247`

| C++ | Status |
|-----|--------|
| `DualLfoLabPanel` | **PARTIAL** — overlay; MOD MATRIX CTA present |

**Priority:** LOW · **Status:** **PARTIAL**

---

## PHASE 4 — iPad adaptation (1194×834)

| View | Node | Status |
|------|------|--------|
| iPad Play | `4:2472` | **NOT STARTED** |
| iPad Design | `4:2662` | **NOT STARTED** |
| iPad Arp | `4:3446` | **NOT STARTED** |
| iPad LFO | `4:3726` | **NOT STARTED** |
| iPad Vocoder | `4:4127` | **NOT STARTED** |

Excluded from desktop implementation scope per `FIGMA_UI_AUDIT.md`.

**Phase 4 status:** **NOT STARTED**

---

## PHASE 5 — Theme system

Five skins (Ivory, Ember, Polar, Chlorophyll, Solarflare) + compact previews.

| Source | Node | Status |
|--------|------|--------|
| Theme previews | `10:4`, `10:139`, `10:265`, `10:401`, `10:534` | **DESIGN-ONLY** |
| Full variant frames (20 total) | various | **DESIGN-ONLY** |

Code ships **Obsidian only** (`ObsidianPalette.h`). Multi-theme = future `LookAndFeel` swap.

**Phase 5 status:** **DESIGN-ONLY**

---

## Phase 2 component map (quick reference)

| Shared component | Primary C++ | Figma node(s) | Status |
|------------------|-------------|---------------|--------|
| Header (design) | `DesignModeHeaderBar` | `37:788` | Figma DONE / Code PARTIAL |
| Header (basic play) | `PatchBrowserBar` | `36:5` | PARTIAL |
| Header (advanced play) | `PatchBrowserBar`, `VstTopBar` | `22:3` | PARTIAL |
| Header (compact) | `CompactModeEditor` | `4:1134` | PARTIAL |
| Single knob | `GlowKnob` | `21:4` | PARTIAL |
| Dual knob | `ConcentricGlowKnob` | `21:4` | PARTIAL |
| Triple knob | `TripleGlowKnob` | `21:4` | PARTIAL |
| Engine card | `EngineCard` | `37:830`, `22:44`, `4:38` | PARTIAL |
| Osc picker + context | `EngineOscillatorPicker` | `28:921`–`28:1605` | PARTIAL |
| FX chain | `FxChainStrip` | `35:4`, `22:188` | PARTIAL |
| Status bar | `VstBottomBar` | `37:1507`, `22:414`, `36:226` | PARTIAL |
| Root shell | `MurmurRootEditor` | — | PARTIAL |

---

## Recommended next work (dependency-first)

**Rule:** Finish **Phase 2** shared chrome before adding more **Phase 3** full-page layouts. Labs already exist as overlays; the gap is **code navigation unification** and **design-mode page routing** (Figma prototype nav is **DONE** — see [`FIGMA_UI_AUDIT.md` — Figma Prototype Navigation Map](FIGMA_UI_AUDIT.md#figma-prototype-navigation-map)).

### Critical path NOW

**Phase 2d — Per-oscillator context grids (9 variants)**

Figma: 9 engine-specific context layouts at `28:921`–`28:1605`.  
Code: `EngineOscillatorPicker` has simplified 4-cell strips + 7/9 wireframe previews; EXT wireframe still missing.

### Top 5 concrete tasks (with Figma nodes)

| # | Task | Figma nodes | C++ touchpoints | Depends on |
|---|------|-------------|-----------------|------------|
| **1** | **Per-osc context grids:** implement Figma-faithful 9 variants (start CLS/WT/FM @ `28:921`); add **EXT** wireframe | `28:921`, `28:1281`, `28:1605` | `EngineOscillatorPicker`, `wireframe/*` | Engine card layout stable |
| **2** | **Inter font embed** or explicit caption tier mapping | tokens | `ObsidianFonts.h` | Phase 1 |
| **3** | **12-slot FX chain** (or document 7-slot as intentional MVP delta) | `35:4` | `FxChainStrip`, `DesignFxSignalChain` | Design FX page |
| **4** | **FX routing bar:** pre/post insert order + send buses | `35:4`, `22:188` | `DesignFxPanel`, `Engine.cpp`, `FxChainFlowView` | Global wet/bypass **DONE** |
| **5** | **`obsidian-play-board` (`22:2`) pixel polish** — engine stub knobs, dashboard meters | `22:2`, `22:188` | `PlayModeEditor`, `EngineGridPanel` | — |

### Suggested order after header

1. `obsidian-play-board` (`22:2`) pixel polish — highest traffic Advanced PLAY surface ([`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md) Part 2).
2. Per-osc context grids (`28:921`–`28:1605`) — replace simplified 4-cell strips with Figma layouts.
3. Lab overlays polish — harmonic editor DSP hookup for wavetable (`27:709`); vocoder band analyzer real FFT tap (future).

---

## Appendix — Original build order (verbatim)

```
PHASE 1 — DESIGN TOKENS & PRIMITIVES
  - Color palette: BG (#0A1114), Header (#0A0E12), Teal (#00FFD0), Dim (#596166), White (#E6EBED)
  - Pill/tab states: active (#004033), inactive (#141A1F @ 60%)
  - Typography: Inter Black/Bold/Medium, 7-11px range
  - Corner radii: 3px (compact pills), 4px (standard pills), 6px (header bars)
  - Spacing: 4/6/8/12/16px increments

PHASE 2 — SHARED COMPONENTS (bottom-up)
  2a. Header system — 37:787, 36:4, 4:1134
  2b. Glow ring knobs — 21:4, 21:172
  2c. Engine card — 37:787, 28:1953, 31:4
  2d. Per-osc context grids (9 variants) — 28:921, 28:1281, 28:1605
  2e. FX slot chip — 35:4, 17:4
  2f. Status/footer bar — 37:787

PHASE 3 — VIEW ASSEMBLY
  3a. COMPACT — 4:1134
  3b. PLAY — 36:4
  3c. DESIGN ENGINE — 37:787
  3d. DESIGN ARP — 4:1267
  3e. DESIGN VOCODER — 15:4
  3f. DESIGN FX — 35:4
  3g. DESIGN MOD MATRIX — 27:265
  3h. PRESET BROWSER — 27:6
  3i. WAVETABLE EDITOR — 27:709
  3j. DUAL LFO — 15:247

PHASE 4 — iPAD — 4:2472, 4:2662, 4:3446, 4:3726, 4:4127
PHASE 5 — THEMES — 10:4, 10:139, 10:265, 10:401, 10:534
```

---

## Related docs

- [`QUASAR_FIGMA_BUILD_GUIDE.md`](QUASAR_FIGMA_BUILD_GUIDE.md) — standalone QUASAR plugin screen (separate from MURMUR frames; reuses Obsidian tokens)
- [`FIGMA_UI_AUDIT.md`](FIGMA_UI_AUDIT.md) — per-frame pixel deltas, child trees, and **Part 6 full node registry** (core views + MI tracks + master envelope)
- [`MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md`](MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md) — MI Tracks A–G implementation plan
- [`UI.md`](UI.md) — OBSIDIAN skin architecture and PLAY mode history
- [`UI_PAGED_LAYOUT.md`](UI_PAGED_LAYOUT.md) — legacy paged PLAY plan (superseded by Alt UI direction)
