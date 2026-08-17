# Figma UI Audit — Obsidian Spec Mode

**File:** `PFt0LG6XmOiZWcSoUXIWIg` (MURMUR Alt UI · canonical direction)  
**Audit date:** 2026-08-17  
**Mode:** Figma-as-spec (numbers copied from `get_metadata` / `get_design_context`, not interpreted)

**Build map & status cross-reference:** [`OBSIDIAN_BUILD_MAP.md`](OBSIDIAN_BUILD_MAP.md) — dependency-first Phases 1–5, C++ component owners, and `DONE | PARTIAL | NOT STARTED | DESIGN-ONLY` per item.

**Navigation status (2026-08-17):**

| Layer | Status | Notes |
|-------|--------|-------|
| **Figma (design)** | **DONE** | Prototype navigation complete — every header tab is a real clickable nav link in prototype mode; active tab correctly excluded (no self-navigation) |
| **Code (implementation)** | **PARTIAL (~75%)** | Unified `MurmurChromeBar` wires COMPACT/PLAY/DESIGN + design sub-nav; lab pages embedded in `DesignModeEditor` |

Excluded from inventory per user request: `ipad-*` frames, theme variant frames (`themes`, `ivory`, `ember`, etc.).

---

## Part 0 — Phase 3 view inventory (build map alignment)

Maps [`OBSIDIAN_BUILD_MAP.md`](OBSIDIAN_BUILD_MAP.md) Phase 3 desktop surfaces to this audit. Advanced PLAY (`22:2`) is implemented in code but not in the original phase list.

| Build map | Figma frame | Node ID | C++ owner(s) | Status |
|-----------|-------------|---------|--------------|--------|
| **3a** COMPACT | `murmur-play-compact` | `4:1134` | `CompactModeEditor` | **PARTIAL** — rectangular scope + stereo meters, footer mod chips + CPU/voices wired; 14px margin / 565px / 152px scope / 32px chrome aligned |
| **3b** PLAY (Basic) | `murmur-desktop-play-mode` | `36:4` | `PlayModeEditor` Basic, `OscilloscopeView`, `PatchFocusPanel`, `MurmurChromeBar` | **PARTIAL** — unified 44px chrome wired; content heights verified |
| **3b-adj** PLAY (Advanced) | `obsidian-play-board` | `22:2` | `PlayModeEditor` Advanced, `EngineGridPanel`, `DashboardStrip` | **PARTIAL** |
| **3c** DESIGN → ENGINE | `murmur-design-engine` | `37:787` | `DesignModeEditor`, `MurmurChromeBar`, `EngineGridPanel`, `EngineCard` | **PARTIAL** — ENGINE sub-page wired; taller cards + sub-picker aligned |
| **3d** DESIGN → ARP | `murmur-design-arp` | `4:1267` | `ArpPanelOverlay`, `ArpStepStrip` | **PARTIAL** — layout constants aligned; routing subtitles + footer MOD chips + hold lock wired |
| **3e** DESIGN → VOCODER | `murmur-vocoder-lab` | `15:4` | `VocoderLabPanel` | **PARTIAL** — 110px signal diagram, 16-band split viz, 680px controls, 32px footer; `← DESIGN` embed |
| **3f** DESIGN → FX | `murmur-design-fx` | `35:4` | `DesignFxPanel`, `DesignFxSignalChain`, `FxChainStrip`, `DesignFxHeroViz` | **PARTIAL** — full-width 1248×360 detail; see **Part 2j** per-FX hero screens |
| **3f′** DESIGN → FX (per-slot) | `murmur-fx-*` (10 frames) | `63:8`…`63:3309` | `DesignFxHeroViz` + chip-scoped knob grid | **DONE** — hero viz, presets, MOOD→Eq DSP, LIM true-peak toggle |
| **3g** DESIGN → MOD MATRIX | `murmur-mod-matrix` | `27:265` | `DesignModMatrixPanel`, `ModRoutingOverlay` (PLAY overlay) | **PARTIAL** — full-page grid wired; PLAY overlay unchanged |
| **3h** PRESET BROWSER | `murmur-preset-browser` | `27:6` | `PresetBrowserOverlay`, `MurmurChromeBar` | **PARTIAL** — 3-column 240/696/300 layout; BROWSE tab; chrome pass-through |
| **3i** WAVETABLE EDITOR | `murmur-wavetable-editor` | `27:709` | `WavetableLabPanel`, `WavetableStackView` | **PARTIAL** — frame strip (8 minis) + morph pills wired; unison VOICES/DETUNE/WIDTH read-only from patch (no APVTS) |
| **3j** DUAL LFO | `murmur-dual-lfo-lab` | `15:247` | `DualLfoLabPanel` | **PARTIAL** — 5·8 tab shows 4-column LFO 5–8; footer LF01–08 chips switch pair tab + highlight active LFO |

**Recent work (2026-08-17):** Milestone B session — COMPACT (`4:1134`) literals aligned (14px margin, 565px default height, 152px scope); duplicate mission-card chrome removed from `CompactModeEditor`. Basic PLAY (`36:4`) vertical budget verified with unified 44px `MurmurChromeBar` (preset + BPM + MASTER OUT label); `PatchBrowserBar` desktop path remains hidden. Design lab panels use `← DESIGN` back copy and 56px header when embedded. Design FX page (`35:4`) now uses `DesignFxPanel` with 12-slot `DesignFxSignalChain`, detail editor via `FxChainStrip::setDesignFxPageMode`, rack overview meters, and routing bar. Design MOD MATRIX page (`27:265`) now uses `DesignModMatrixPanel` with full-width 11×8 interconnect grid (70/105px columns), active routing paths list, quick-config sidebar, and status bar; PLAY overlay (`ModRoutingOverlay`, `m` key) unchanged.

**Recent work (2026-08-17, design FX push):** `DesignFxPresetLibrary` loads `content/design-fx/*.json` (37 chip presets). Interactive EQ graph with band ordering + Shift+Q. Live FFT analyzer when ANALYZER ACTIVE. Limiter hero PK meter + CLIP animation. `DesignFxStubKnob` + `DesignFxUiState` for ui-only knobs feeding `DesignFxHeroViz`. MOOD maps ui knobs → Eq DSP on insert slot I3. APVTS wired: COMP MAKEUP, FRAC SCATTER/PITCH, CHR SPREAD, TAPE HISS/AGE, FSHF SCALE, VOC SIBILANCE. User preset save/delete/rename under `content/design-fx/user/`. Optional `pw8Ref` sidecars merge factory `.pw8` slot params without full patch load. Signal-chain drag-to-reorder (BYP fixed at head). Fixed `TapeDrive` param id (was invalid `TapeDriveDb`).

**Recent work (2026-08-17, Figma-as-spec batch 3):** COMPACT (`4:1134`) — `OscilloscopeView::paintCompactMode` rectangular scope panel (grid, waveform, SIGNAL IN LED); stereo output meters via `ScopeVuMeter`; footer LFO1/ENV/SEQ/RAND mod chips with route highlight; live CPU/voices in footer. WAVETABLE (`27:709`) — center-column frame strip (8×70×50 minis, `WavetablePos` APVTS); morph type pills SPECTRAL/FORMANT/CROSSFADE (UI-only, subtitle). DUAL LFO (`15:247`) — 5·8 pair tab 4-column layout (LFO 5–8); footer LF01–08 `TextButton` chips wired via `selectFooterLfo`. DESIGN FX (`35:4`) — overview OUT/GR meters from slot Mix + scope peak; routing bar global wet mix averaged from active FX Mix params; sidechain chip uses `getSidechainActive()`.

**Recent work (2026-08-17, lab polish session):** Design VOCODER (`15:4`) — `VocoderLabPanel` rebuilt to Figma layout: 110px routing signal-flow diagram, 16-band split analyzer (carrier/modulator), 680px right controls card (2×3 knobs + chain routing bar + OPEN FULL FX CHAIN), 32px logic footer; embedded `← DESIGN` header. Design WAVETABLE (`27:709`) — `WavetableLabPanel` 3-column layout (240 / flex / 280), 24px subtitle bar, 256×336 harmonic partials editor paint, 40px status footer with live CPU/voices via `PerformanceMetricsUi.h`. Design DUAL LFO (`15:247`) — `DualLfoLabPanel` dual-column layout with 180px scopes, routing footers, 56px bottom bar with LF01–08 quick-link chips and OPEN MOD MATRIX CTA. `MurmurChromeBar` preset chevrons already wired to `stepPreset()`.

**Recent work (2026-08-17, Phase 3 push):** Cross-cutting — `DesignFxPanel` overview footer now shows live CPU bar + voice count via `PerformanceMetricsUi.h`; `VstBottomBar` design-mode MOD chips (LFO1/LFO2/ENV1/ENV2) highlight active mod routes like ARP footer. WAVETABLE left column (`27:754`) — source waveform list (SINE→WAVETABLE), COARSE/FINE decked knobs, unison readouts from patch, phase dispersion toggle. DUAL LFO — header pair tabs LFO 1·2 / 3·4 / 5·8 rebind columns; sync division chip row (`15:325`) wired to `SyncDivisionIndex` per LFO.

**Recent work (2026-08-17, Figma-as-spec batch 4):** Engine deep editor (`28:4`) — `EngineDetailOverlay` rebuilt to Figma 3-column OSC / Filter / Amp layout with 8×28px ENG tab pills, ON/SOLO/MUTE APVTS, letterboxed 1440×1024 embed; `kEngineDeepEditor*` constants. Preset browser (`27:6`) — `PresetBrowserOverlay` literal 240 / 696 / 300 layout with category tree, table columns, detail profile; chrome pass-through for `MurmurChromeBar` nav.

**Recent work (2026-08-17, engine + UI push):** `WtMorphMode` DSP — Spectral (nearest frame), Formant (equal-power crossfade), Crossfade (linear) in `WavetableOscillator`. Preset browser LIST view scroll for long tables. Vocoder band analyzer uses frequency-weighted sidechain envelope. Advanced PLAY mod-assignment banner reserves layout space (no grid overlap). Build map completion revised to ~68% overall (~75% Phase 3).

---

## Part 1 — Frame inventory

| Frame | Node ID | Size (Figma) | C++ owner(s) | figma-connect stub | Status | Top 3 spec gaps |
|-------|---------|--------------|--------------|-------------------|--------|-----------------|
| **murmur-design-engine** | `37:787` | 1280×720 | `DesignModeEditor`, `MurmurChromeBar`, `EngineGridPanel`, `EngineCard`, `EngineOscillatorPicker`, `EngineAdsrMini`, `VstBottomBar`; mode switch via `MurmurRootEditor` | — | **Figma DONE** / **Code PARTIAL** | Renamed from `murmur-design-mode-v2`; 270px cards + 22px sub-picker + 80px context viz; chrome 62px via `MurmurChromeBar` |
| **obsidian-play-board** | `22:2` | 1280×720 | `PlayModeEditor`, `EngineGridPanel`, `DashboardStrip`, `VstBottomBar`; chrome via `MurmurRootEditor` → `PatchBrowserBar`, `VstTopBar` | — (board-level); `EngineCard.figma.ts` maps cards | **PARTIAL** | play-board stub knobs reflect live coarse/fine; FX flow shows PRE/POST tap |
| **murmur-desktop-play-mode** | `36:4` | 1280×720 | `PlayModeEditor` Basic view, `OscilloscopeView`, `PatchFocusPanel`, `MurmurChromeBar`, `VstBottomBar` | — | **PARTIAL** | Scope bar viz vs waveform path; stage latch not wired |
| **murmur-8-engine-vst** | `4:4` | 1440×1024 | `EngineGridPanel`, `EngineCard`, `FxChainStrip`, `FilterLfoPanel` | `EngineCard.figma.ts` | **PARTIAL** | Reference is 1440×1024 not 1280×720; card anatomy constants in `PlayModeLayout.h` |
| **murmur-design-arp** | `4:1267` | 1280×720 | `ArpPanelOverlay`, `ArpStepStrip` | `ArpPanelOverlay.figma.ts` | **PARTIAL** | Design embed uses 56px lab header + `← DESIGN`; Figma frame uses unified 62px chrome only |
| **murmur-vocoder-lab** | `15:4` | 1280×720 | `VocoderLabPanel` | `VocoderLabPanel.figma.ts` | **PARTIAL** | Sidechain node labels static; band analyzer uses animated placeholder not live FFT |
| **murmur-dual-lfo-lab** | `15:247` | 1280×720 | `DualLfoLabPanel` | `DualLfoLabPanel.figma.ts` | **PARTIAL** | 5·8 tab 4-column layout done; narrow columns may need horizontal scroll polish vs Figma |
| **murmur-mod-matrix** | `27:265` | 1280×720 | `DesignModMatrixPanel`, `ModRoutingOverlay` (PLAY), `ModRoutingUi` | `ModRoutingOverlay.figma.ts` (legacy file ref) | **PARTIAL** | Grid 11×8 literal columns; KEY TRACK/RANDOM rows display-only; curve/MIDI learn display-only |
| **murmur-preset-browser** | `27:6` | 1280×720 | `PresetBrowserOverlay`, `PatchBrowserBar` | `PresetBrowserOverlay.figma.ts` (legacy node) | **PARTIAL** | 3-column 240/696/300 aligned; AUTHOR from preset metadata JSON; GRID scroll wired; LIST scroll TBD; EXPORT display-only |
| **murmur-engine-deep-editor** | `28:4` | 1440×1024 | `EngineDetailOverlay`, `OperatorEditorPanel`, `FilterLfoPanel`, `EngineAdsrMini` | `EngineCard.figma.ts` (partial) | **PARTIAL** | 3-column OSC/Filter/Amp; level/pan APVTS; live mod routes; unison voices/detune/spread APVTS wired |
| **murmur-play-compact** | `4:1134` | 320×565 | `CompactModeEditor`, `MurmurChromeBar` | — | **PARTIAL** | Mod chip route detection is heuristic (LFO1/ENV only); SEQ/RAND display-only |
| **murmur-design-fx** | `35:4` | 1280×720 | `DesignFxPanel`, `DesignFxSignalChain`, `FxChainStrip` | — | **PARTIAL** | SAT stub APVTS wired; insert pre/post + send A/B engine buses **DONE** |
| **murmur-play-fx-rack** | `15:478` | *(sub-frame)* | `DashboardStrip`, `FxChainStrip`, `FilterLfoPanel` | `FxChainStrip.figma.ts`, `FilterLfoPanel.figma.ts` | **PARTIAL** | Vocoder slot editor content density at 94px; sidechain CTA chip layout |
| **murmur-wavetable-editor** | `27:709` | 1280×720 | `WavetableLabPanel`, `WavetableStackView` | `WavetableLabPanel.figma.ts` | **PARTIAL** | Morph type + unison knobs APVTS wired; harmonic editor paint-only; **WtMorphMode DSP wired** (Spectral/Formant/Crossfade) |
| **glow-ring-knobs** | `21:4` | — | `GlowKnob`, `GlowRingButton`, `ConcentricGlowKnob`, `TripleGlowKnob` | `GlowKnob.figma.ts`, `GlowRingButton.figma.ts`, `ConcentricGlowKnob.figma.ts`, `TripleGlowKnob.figma.ts` | **PARTIAL** | Knob sizes vary by context (28/30/36/44px); glow arc stroke weights not tokenized |
| **glow-knob-interaction-concepts** | `21:172` | — | — | — | **DESIGN-ONLY** | Interaction/motion reference only; no C++ surface |
| **murmur-8-engine-cover** | `27:1115` | 1440×900 | — | — | **DESIGN-ONLY** | File index / navigation; not a runtime screen |

### Component atoms (supporting frames, not full screens)

| Frame / component | Node | C++ owner | figma-connect | Status |
|-------------------|------|-----------|---------------|--------|
| `vst-top-bar` (in play-board) | `22:3` | `PatchBrowserBar`, `VstTopBar` | — | PARTIAL |
| `engine-grid-placeholder` | `22:37` | `EngineGridPanel` | `EngineOscillatorPicker.figma.ts` | PARTIAL |
| `dashboard-strip-outer` | `22:188` | `DashboardStrip` | `FxChainStrip.figma.ts` | PARTIAL |
| `status-bar` | `22:414` | `VstBottomBar` | — | PARTIAL |
| `left-parameters` (ARP) | `4:1305` | `ArpPanelOverlay` side column | — | PARTIAL |
| `step-sequencer-row` | `4:1353` | `ArpStepStrip` | — | PARTIAL |
| `global-filter-panel` | `22:376` | `FilterLfoPanel` (dashboard mode) | `FilterLfoPanel.figma.ts` | PARTIAL |
| `LiveTopologyStrip` | *(legacy pi5k file)* | `LiveTopologyStrip` | `LiveTopologyStrip.figma.ts` | PARTIAL |
| `AlgorithmGraphView` | *(legacy pi5k file)* | `AlgorithmGraphView` | `AlgorithmGraphView.figma.ts` | PARTIAL |
| `TopologyGraphOverlay` | *(legacy pi5k file)* | `TopologyGraphOverlay` | `TopologyGraphOverlay.figma.ts` | PARTIAL |
| `SectionPanel` | *(legacy pi5k file)* | `SectionPanel` | `SectionPanel.figma.ts` | DONE |
| `ModSourceChip` | *(legacy pi5k file)* | `ModSourceChip` | `ModSourceChip.figma.ts` | PARTIAL |
| `GlobalPanel` | *(legacy pi5k file)* | `GlobalPanel` | `GlobalPanel.figma.ts` | PARTIAL |
| `WireframePanel` | *(legacy pi5k file)* | `WireframePanel` | `WireframePanel.figma.ts` | PARTIAL |

**Note:** Several figma-connect stubs still point at legacy file `pi5kUNcZWQGhqfRAVu3voh`; canonical frames live in `PFt0LG6XmOiZWcSoUXIWIg`. Update URLs when running Code Connect publish.

---

---

## Part 2c — `murmur-design-engine` (37:787) delta table

**Note:** Renamed from `murmur-design-mode-v2` (same node `37:787`). ENGINE sub-page is **8 engine cards only** — no FX rack. Chrome is unified 62px `MurmurChromeBar` (`39:2`); cards grew from 174→270px with new **sub-picker** row (`47:*`).

Figma vertical budget at 1280×720 (from metadata node `37:787`):

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 1280×720 | `kDefaultWidth/Height` | `PlayModeLayout.h` | N |
| Outer padding | 16 | `kDesignModeV2OuterMargin = 16` | `PlayModeLayout.h`, `MurmurRootEditor.cpp` | N |
| Section gap | 12 | `kDesignModeV2SectionGap = 12` | `PlayModeLayout.h`, `DesignModeEditor.cpp` | N |
| **Chrome: header-bar** | 62 (`39:2`) | `kChromeBarHeightDesign = 62` | `PlayModeLayout.h`, `MurmurChromeBar` | N |
| **Grid section** | 560 (`37:830`) | `kDesignModeV2GridSectionHeight = 560` | `PlayModeLayout.h`, `DesignModeEditor.cpp` | N (2026-08-17) |
| Grid row height | 270 | `kDesignModeV2GridRowHeight = 270` | `PlayModeLayout.h` | N (2026-08-17) |
| Grid row gap | 12 | `kDesignModeV2GridRowGap = 12` | `PlayModeLayout.h`, `EngineGridPanel.cpp` | N |
| Grid col gap | 12 | `kDesignModeV2GridColGap = 12` | `PlayModeLayout.h`, `EngineGridPanel.cpp` | N |
| Engine card size | 303×270 | cell fill in 4×2 grid | `EngineGridPanel.cpp` | N (2026-08-17) |
| Card padding | 12 | `kDesignModeV2CardPadding = 12` | `PlayModeLayout.h`, `EngineCard.cpp` | N |
| Card row gap | 10 | `kDesignModeV2CardRowGap = 10` | `PlayModeLayout.h`, `EngineCard.cpp` | N |
| Card header | 14 | `kDesignModeV2CardHeaderHeight = 14` | `PlayModeLayout.h`, `EngineCard.cpp` | N |
| Type strip | 14 | `kDesignModeV2TypeStripHeight = 14` | `PlayModeLayout.h`, `EngineOscillatorPicker.cpp` | N |
| **Sub-picker row** | 22 (`47:2`) | `kDesignModeV2SubPickerHeight = 22` | `PlayModeLayout.h`, `EngineOscillatorPicker.cpp` | N (2026-08-17) |
| Sub-picker pills | 14 | `kDesignModeV2SubPickerPillHeight = 14` | `PlayModeLayout.h` | N (2026-08-17) |
| Context visualizer | 80 | `kDesignModeV2ContextVisualizerHeight = 80` | `PlayModeLayout.h`, `EngineOscillatorPicker.cpp` | N (2026-08-17) |
| Oscillator picker total | 136 | `kDesignModeV2OscillatorPickerHeight` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Knobs + envelope row | 40 | `kDesignModeV2KnobsEnvelopeRowHeight = 40` | `PlayModeLayout.h`, `EngineCard.cpp` | N |
| ADSR mini | 68×34 | `kDesignModeV2EnvelopeWidth/Height` | `PlayModeLayout.h`, `EngineCard.cpp` | N |
| Level row | 10 | `kDesignModeV2LevelRowHeight = 10` | `PlayModeLayout.h`, `EngineCard.cpp` | N |
| **Status bar** | 40 (`37:1507`) | `kDesignModeV2StatusBarHeight = 40` | `PlayModeLayout.h`, `DesignModeEditor.cpp` | N |
| Status bar padding X | 16 | `kDesignModeV2StatusBarPaddingX = 16` | `PlayModeLayout.h`, `VstBottomBar.cpp` | N |
| MOD source chips | LFO1/LFO2/ENV1/ENV2 | route-highlight paint in `VstBottomBar::paintOverChildren` | `VstBottomBar.cpp` | N (2026-08-17) |
| CPU meter | decorative 40% | live CPU bar + % via `PerformanceMetricsUi.h` | `VstBottomBar.cpp` | N (2026-08-17) |
| Preset chevrons | absent in `39:2` | `MurmurChromeBar` preset row + `stepPreset()` | `MurmurChromeBar.cpp` | N (2026-08-17) |
| Design sub-nav tabs | ENGINE/ARP/VOC/FX/MOD | wired via `MurmurChromeBar` | `MurmurRootEditor.cpp` | N |
| Per-engine sub-picker variants | 8 engine-specific layouts | 9 variants wired in `EngineOscillatorPicker.cpp` | `EngineOscillatorPicker.cpp` | N (2026-08-17) |
| Per-engine context visualizer | 9 engine-specific 80px layouts | painted per engine type | `EngineOscillatorPicker.cpp` | N (2026-08-17) |
| Sub-picker display-only gaps | — | EXT input, GRN sample load, WT morph proxy via WtBend | `EngineOscillatorPicker.cpp` | **Y** (3 display-only) |
| FX rack / dashboard | absent in frame | excluded from `DesignModeEditor` | `DesignModeEditor.cpp` | N |

### Child tree (Figma metadata)

```
murmur-design-engine (37:787) 1280×720
├── header-bar (39:2) 1248×62 @ (16,16)
│   ├── header-top-row (39:3) 1216×23 — logo, COMPACT/PLAY/DESIGN, preset info
│   └── design-subnav (39:19) 1216×22 — ENGINE/ARP/VOCODER/FX/MOD MATRIX
├── grid-section (37:830) 1248×560 @ (16,90)
│   ├── grid-row-1 (37:831) 1248×270
│   │   └── engine-card-1..4 (37:832+) 303×270
│   │       ├── header-row (14) + state-controls ON/S/M
│   │       ├── type-strip (14) — 9 engine-type pills
│   │       ├── sub-picker (47:*) (22) — waveform/sub-type pills or cycler
│   │       ├── context-visualizer-area (80)
│   │       ├── knobs-and-envelope-row (40)
│   │       └── level-row (10)
│   └── grid-row-2 (37:1173) 1248×270 (+12 gap)
│       └── engine-card-5..8
└── status-bar (37:1507) 1248×40 @ (16,662)
    ├── tech-info — CPU, VOICES, MIDI IN
    ├── mod-sources — LFO1/LFO2/ENV1/ENV2 chips
    └── panic-action — PANIC RESET
```

### Changes vs old `murmur-design-mode-v2` repair

| Aspect | Old (`37:787` as design-mode-v2) | New (`murmur-design-engine`) |
|--------|----------------------------------|------------------------------|
| Frame name | `murmur-design-mode-v2` | `murmur-design-engine` (same node) |
| Header node | `37:788` 54px standalone | `39:2` 62px unified chrome + sub-nav |
| Grid section height | 360px @ y=82 | 560px @ y=90 |
| Card height | 174px | 270px (+96px) |
| Sub-picker | absent | 22px row between type-strip and context |
| Context visualizer | 32px | 80px (+48px) |
| Status bar position | y=454 | y=662 (unchanged 40px height) |

---

## Part 2f — `murmur-design-fx` (35:4) delta table

Figma vertical budget at 1280×720 (from `get_metadata` node `35:4`). Unified chrome (`39:86`, 62px) is owned by `MurmurChromeBar`; content below uses `DesignFxPanel`.

**Status (2026-08-17 gap closure):** Part 2j per-FX hero editors are **DONE** in `DesignFxPanel` (full-width detail, no overview sidebar). Part 2f rows below reflect the **shared** FX rack chrome only.

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 1280×720 | `kDefaultWidth/Height` | `PlayModeLayout.h` | N |
| Outer padding | 16 | `kDesignModeV2OuterMargin = 16` | `PlayModeLayout.h`, `MurmurRootEditor.cpp` | N |
| Section gap | 12 | `kDesignFxPageSectionGap = 12` | `PlayModeLayout.h`, `DesignFxPanel.cpp` | N |
| **Chrome: header-bar** | 62 (`39:86`) | `kChromeBarHeightDesign = 62` | `PlayModeLayout.h`, `MurmurChromeBar` | N |
| Embedded lab header | absent in frame | `kDesignLabPanelHeaderHeight = 56` | `DesignFxPanel.cpp` | **Y** (code nav addition) |
| **Signal chain section** | 119 (`35:50`) | `kDesignFxPageSignalChainSectionHeight = 119` | `PlayModeLayout.h`, `DesignFxPanel.cpp` | N |
| Label row | 11 | `kDesignFxPageSignalChainLabelHeight = 11` | `PlayModeLayout.h` | N |
| Flow pipeline | 94 (`35:54`) | `kDesignFxPageSignalChainPipelineHeight = 94` | `PlayModeLayout.h` | N |
| FX chip tile | 88×82 | `kDesignFxPageChipWidth/Height` | `PlayModeLayout.h`, `DesignFxSignalChain.cpp` | N |
| FX chip count | 12 (BYP→VOC) | `kDesignFxPageSlotCount = 12` | `PlayModeLayout.h` | N |
| Flow connector | 12 | `kDesignFxPageFlowConnectorWidth = 12` | `PlayModeLayout.h` | N |
| **Middle workspace** | 417 (`35:245`) | flex fill — **full-width detail** (no 380px overview) | `DesignFxPanel.cpp` | N |
| Detail panel | 1248×360 (Part 2j) | flex fill | `DesignFxPanel.cpp` | N |
| Detail padding | 16 | `kDesignFxPageDetailPadding = 16` | `PlayModeLayout.h` | N |
| Focused header | 18 | `kDesignFxPageDetailHeaderHeight = 18` | `PlayModeLayout.h` | N |
| Knob cell | 64×60 | `kDesignFxPageDetailKnobWidth/Height` | `PlayModeLayout.h`, `FxChainStrip.cpp` | N |
| Overview panel | 380×360 (`35:349`) | **removed** — Part 2j full-width detail | — | N (by design) |
| **Routing bar** | 54 (`35:455`) | `kDesignFxPageRoutingBarHeight = 54` | `PlayModeLayout.h`, `DesignFxPanel.cpp` | N |
| Sidechain chip | 109×18 | painted; `getSidechainActive()` tint | `DesignFxPanel.cpp` | N |
| Global wet mix slider | 140×14 | `fxGlobalWetMix` APVTS + drag | `DesignFxPanel.cpp` | N |
| Send A/B knobs | 22×18 | `fxSendA` / `fxSendB` APVTS + drag | `DesignFxPanel.cpp` | N (2026-08-17) |
| PRE/POST + ALL BYPASS | 90+83 | `fxRoutingPrePost` insert order + send tap / `fxGlobalBypass` APVTS | `DesignFxPanel.cpp`, `Engine.cpp` | N (2026-08-17) |
| Drag-to-reorder | label present | `DesignFxSignalChain` + `DesignFxUiState` + `fxProcessOrder` engine permute | `DesignFxSignalChain.cpp` | N (2026-08-17) |
| FX sprite glyphs (`17:4`) | asset sheet | procedural mini-glyphs (SAT/CHR/TAPE/FSHF/FRAC/…) | `DesignFxSignalChain.cpp` | N (2026-08-17) |
| Preset dropdown | `35:253` | `DesignFxPresetLibrary` + user save/rename/delete | `DesignFxPanel.cpp` | N |
| Spectrum viz (`35:324`) | EQ curve + grid | live FFT analyzer + interactive graph | `DesignFxHeroViz`, `FxChainStrip` | N |
| CPU / FX load footer | 14.2% | live CPU + FX load via `PerformanceMetricsUi.h` | `DesignFxSignalChain.cpp`, `VstBottomBar.cpp` | N (2026-08-17) |

### Child tree (Figma metadata)

```
murmur-design-fx (35:4) 1280×720
├── header-bar (39:86) 1248×62 @ (16,16)          → MurmurChromeBar
├── fx-signal-chain-container (35:50) 1248×119    → DesignFxSignalChain
│   ├── label-row (35:51) 1248×11
│   └── flow-pipeline (35:54) 1248×94
│       ├── terminal-input (35:55)
│       ├── pipeline-modules (35:59) — 12× fx-slot-* (88×82 tiles)
│       └── terminal-output (35:241)
├── middle-workspace (35:245) 1248×417
│   ├── selected-fx-detail-panel (35:246) 856×360 → DesignFxPanel + FxChainStrip
│   └── fx-overview-meters-panel (35:349) 380×360   → DesignFxPanel::paintOverviewPanel
└── vst-bottom-bar (35:455) 1248×54                 → DesignFxPanel::paintRoutingBar
```

---

## Part 2j — Per-FX editing screens (`murmur-fx-*`, node `63:*`)

Ten new **1280×720** prototype destinations (file `PFt0LG6XmOiZWcSoUXIWIg`). Each is a full DESIGN-mode page: unified chrome + **same 12-slot signal chain** as `35:4`, but the middle workspace is a **single full-width detail panel** (1248×360) — **no overview meters sidebar**.

Prototype intent: tap a signal-chain chip on `murmur-design-fx` → navigate to the matching `murmur-fx-*` frame (in-place swap in code).

### Frame inventory

| Frame | Node | Selected chip | Detail title (Figma) | Visualizer |
|-------|------|---------------|----------------------|------------|
| `murmur-fx-saturation` | `63:8` | SAT (I2) | SATURATION HARMONIC ENGINE (SLOT I2) | Transfer curve (input vs output) |
| `murmur-fx-chorus` | `63:368` | CHR (I3) | CHORUS MULTI-VOICE SPATIALIZER (SLOT I3) | Stereo L/R delay taps |
| `murmur-fx-tape` | `63:724` | TAPE (I4) | ANALOG TAPE SATURATION & DRIFT (SLOT I4) | Tape drift / wow visualization |
| `murmur-fx-mood` | `63:1090` | MOOD (I5) | MOOD SPECTRAL RESONATOR FILTER (SLOT I5) | Frequency response morph |
| `murmur-fx-freqshift` | `63:1451` | FSHF (I6) | FREQUENCY SHIFTER & BODE SYSTEM (SLOT I6) | Bode / shift spectrum |
| `murmur-fx-fractal` | `63:1836` | FRAC (I7) | FRACTAL GRANULAR PROCESSOR (SLOT I7) | Granular particle cloud |
| `murmur-fx-reverb` | `63:2227` | REV (I8) | REVERB SPACE DESIGNER (SLOT I8) | Spectral decay envelope |
| `murmur-fx-equalizer` | `63:2590` | EQ (I9) | PARAMETRIC EQUALIZER (SLOT I9) | **Full-width** interactive EQ graph (1082×220) + band sidebar |
| `murmur-fx-compressor` | `63:2934` | COMP (I10) | DYNAMICS COMPRESSOR (SLOT I10) | Input/output + GR dynamics graph |
| `murmur-fx-limiter` | `63:3309` | LIM (I11) | BRICKWALL LIMITER (SLOT I11) | True-peak / ceiling meter |

**Not given a dedicated frame:** BYP (I0), VOC (I12) — remain on chain strip only.

### Shared layout anatomy (all 10)

```
murmur-fx-* (1280×720)
├── header-bar (62px)                    → MurmurChromeBar (DESIGN + FX tab active)
├── fx-signal-chain-container (119px)    → DesignFxSignalChain (chip highlighted)
│   └── label: "LATENCY FLUID SYNC ACTIVE"
├── middle-workspace (419px)
│   └── selected-fx-detail-panel (1248×360)   ← FULL WIDTH (no 380px overview)
│       ├── focused-header (18px): LED + title + preset dropdown + status + ACTIVE toggle
│       ├── knobs-and-visualization (296px): controls-subgrid 280×174 + viz ~448–936px
│       │   ├── 2×3 knob grid (64×58 cells, 32px dials, 44px row gap)
│       │   └── effect-specific wireframe viz (right)
│       └── type/mode selector strip (below knobs): per-effect pills
└── vst-bottom-bar (54px)                → sidechain chip, global wet, send A/B, PRE/POST, ALL BYPASS
```

### Per-effect controls (Figma knob labels)

| FX | Row 1 | Row 2 | Type / mode strip |
|----|-------|-------|-------------------|
| **SAT** | DRIVE, TONE, COLOR | MIX, OUTPUT, BIAS | TUBE / TAPE / DIODE / FOLD / CRUSH |
| **CHR** | RATE, DEPTH, FEEDBACK | MIX, VOICES, SPREAD | — |
| **TAPE** | SPEED, WOW, FLUTTER | SATURATION, HISS, AGE | — |
| **MOOD** | INTENSITY, COLOR, FILTER FREQ | RESONANCE, DRIVE, MIX | WARM / DARK / BRIGHT / ACID / ETHEREAL |
| **FSHF** | SHIFT, FEEDBACK, MIX | SCALE, DETUNE, STEREO | — |
| **FRAC** | DENSITY, GRAIN SIZE, PITCH | SCATTER, POSITION, MIX | — |
| **REV** | PREDELAY, DECAY, SIZE | DAMPING, DIFFUSION, MIX | HALL / PLATE / ROOM / SPRING / SHIMMER |
| **EQ** | band sidebar + OUT GAIN, MIX | *(graph is primary)* | ANALYZER ACTIVE toggle |
| **COMP** | THRESHOLD, RATIO, ATTACK | RELEASE, KNEE, MAKEUP | — |
| **LIM** | CEILING, RELEASE, LOOKAHEAD | ISP, GAIN | TRUE PEAK ACTIVE (hero toggle) |

### Delta vs current code (`DesignFxPanel` / `35:4`)

| Figma (Part 2j) | Code today | Fix? |
|-----------------|------------|------|
| Detail **1248×360** full width | Full-width detail + hero viz (overview sidebar removed) | **DONE** |
| Per-effect **hero visualizer** (448–1082px) | `DesignFxHeroViz` per chip (12 incl. BYP/MOOD) | **DONE** |
| **Type/mode pill strip** under knobs | SAT/REV/COMP/MOOD pills wired; MOOD → Eq DSP | **DONE** |
| **Preset dropdown** in focused header | `DesignFxPresetLibrary` + `content/design-fx/*.json` + user save/rename/delete | **DONE** |
| Signal chain chip → **prototype navigation** | In-place `bindSelectedChip()` + drag-to-reorder display | **DONE** |
| EQ **unique layout** (graph-first) | 134px band sidebar + interactive graph + live FFT analyzer | **DONE** |
| Stub knobs (TONE, MOOD INTENSITY, SAT OUTPUT, etc.) | `DesignFxStubKnob` + `DesignFxUiState` → hero viz + APVTS (`applySat/Chr/Tape/Fshf/Frac` stubs) | **DONE** |
| VOC design page | Full knob row incl. SIBILANCE; animated band hero; `pw8Ref` factory sidecars | **DONE** |

### Recommended code structure

1. **`DesignFxPanel`**: keep signal chain + routing bar; middle area hosts **`DesignFxDetailRouter`**.
2. **`DesignFxDetailRouter`**: factory by selected chip → `SaturationFxView`, `ChorusFxView`, … `LimiterFxView`.
3. **Reuse** `ConcentricGlowKnob` + `PlayModeLayout.h` knob constants (`kDesignFxPageDetailKnob*`).
4. **Wireframe views** under `plugin/src/ui/components/wireframe/` (mirror `WavetableMeshPaint` pattern).
5. **MOOD**: maps ui knobs + pills → Eq on shared insert slot (I3) via `applyMoodKnobsToEq()`.

---

## Part 2g — `murmur-design-mod-matrix` (27:265) delta table

Figma vertical budget at 1280×720 (from `get_metadata` node `27:265`). Unified chrome (`39:114`, 62px) is owned by `MurmurChromeBar`; content below uses `DesignModMatrixPanel`.

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 1280×720 | `kDefaultWidth/Height` | `PlayModeLayout.h` | N |
| Outer padding | 12 | `kDesignModMatrixPageOuterMargin = 12` | `PlayModeLayout.h` | N (2026-08-17) |
| Section gap | 10 | `kDesignModMatrixPageSectionGap = 10` | `PlayModeLayout.h`, `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| **Chrome: header-bar** | 62 (`39:114`) | `kChromeBarHeightDesign = 62` | `PlayModeLayout.h`, `MurmurChromeBar` | N |
| Embedded lab header | absent in frame | `kDesignLabPanelHeaderHeight = 56` | `DesignModMatrixPanel.cpp` | **Y** (code nav addition) |
| **Matrix workspace** | 574 (`27:304`) | flex fill in `DesignModMatrixPanel` | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| Left main area | 966 | flex left of sidebar | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| **Grid card** | 304 (`27:306`) | `kDesignModMatrixPageGridCardHeight = 304` | `PlayModeLayout.h` | N (2026-08-17) |
| Card padding | 12 | `kDesignModMatrixPageCardPadding = 12` | `PlayModeLayout.h` | N (2026-08-17) |
| Card header | 14 | `kDesignModMatrixPageCardHeaderHeight = 14` | `PlayModeLayout.h` | N (2026-08-17) |
| Grid canvas | 258 | painted in card | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| Source column | 70 | `kDesignModMatrixPageSourceColumnWidth = 70` | `PlayModeLayout.h` | N (2026-08-17) |
| Dest column | ~105 | `kDesignModMatrixPageDestColumnWidth = 105` | `PlayModeLayout.h` | N (2026-08-17) |
| Grid row height | ~20 | `kDesignModMatrixPageGridRowHeight = 20` | `PlayModeLayout.h` | N (2026-08-17) |
| Cell dial | 16 | `kDesignModMatrixPageCellDialSize = 16` | `PlayModeLayout.h` | N (2026-08-17) |
| Source rows | 11 | `kDesignModMatrixPageSourceRowCount = 11` | `PlayModeLayout.h` | N (2026-08-17) |
| Dest columns | 8 | `kDesignModMatrixPageDestColumnCount = 8` | `PlayModeLayout.h` | N (2026-08-17) |
| Card gap (grid→routes) | 10 | `kDesignModMatrixPageCardGap = 10` | `PlayModeLayout.h` | N (2026-08-17) |
| **Active routings card** | 260 (`27:567`) | `kDesignModMatrixPageRoutesCardHeight = 260` | `PlayModeLayout.h` | N (2026-08-17) |
| Route row height | 28 | `kDesignModMatrixPageRouteRowHeight = 28` | `PlayModeLayout.h` | N (2026-08-17) |
| Route source col | 80 | `kDesignModMatrixPageRouteSourceWidth = 80` | `PlayModeLayout.h` | N (2026-08-17) |
| Route dest col | 130 | `kDesignModMatrixPageRouteDestWidth = 130` | `PlayModeLayout.h` | N (2026-08-17) |
| Depth track | 522 | `kDesignModMatrixPageRouteDepthTrackWidth = 522` | `PlayModeLayout.h` | N (2026-08-17) |
| Curve selector | 27×16 | painted LIN/EXP/LOG/S | `DesignModMatrixPanel.cpp` | **Y** (display-only) |
| Polar toggle | 17×16 / 27×16 | painted + / +/- | `DesignModMatrixPanel.cpp` | **Y** (display-only) |
| **Quick config sidebar** | 280 (`27:655`) | `kDesignModMatrixPageSidebarWidth = 280` | `PlayModeLayout.h` | N (2026-08-17) |
| Sidebar gap | 10 | `kDesignModMatrixPageSidebarGap = 10` | `PlayModeLayout.h` | N (2026-08-17) |
| Drag source card | 256×76 | painted + `ModSourceChip` pills | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| Source pill | 32×18 | `kDesignModMatrixPageQuickConfigPillWidth/Height` | `PlayModeLayout.h` | N (2026-08-17) |
| Recent destinations | 256×106 | painted from live routes | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| MIDI learn card | 256×111 | painted button | `DesignModMatrixPanel.cpp` | **Y** (not wired) |
| KEY TRACK / RANDOM rows | present | display-only (no mod source) | `DesignModMatrixPanel.cpp` | **Y** |
| **Status bar** | 40 (`27:694`) | `kDesignModMatrixPageStatusBarHeight = 40` | `PlayModeLayout.h`, `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| CPU meter | 40% decorative | live CPU bar + % via `PerformanceMetricsUi.h` | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| Copyright line | right-aligned | painted | `DesignModMatrixPanel.cpp` | N (2026-08-17) |
| PLAY overlay (`m` key) | N/A (separate flow) | `ModRoutingOverlay` 88% fill | `ModRoutingOverlay.cpp` | N (by design) |

### Child tree (Figma metadata)

```
murmur-design-mod-matrix (27:265) 1280×720
├── header-bar (39:114) 1248×62 @ (12,12)          → MurmurChromeBar
├── matrix-workspace (27:304) 1256×574 @ (12,84)
│   ├── left-main-area (27:305) 966×574
│   │   ├── matrix-grid-card (27:306) 966×304       → DesignModMatrixPanel::paintInterconnectGrid
│   │   └── active-routings-card (27:567) 966×260    → DesignModMatrixPanel::paintActiveRoutes
│   └── right-assign-panel (27:655) 280×574         → DesignModMatrixPanel::paintQuickConfigSidebar
└── vst-bottom-bar (27:694) 1256×40 @ (12,668)      → DesignModMatrixPanel::paintStatusBar
```

---

---

## Part 2h — `murmur-design-preset-browser` (27:6) delta table

Figma layout at 1280×720 (from `get_metadata` node `27:6`). Chrome (`51:2`, 60px) owned by `MurmurChromeBar`; overlay content insets via `hitTest` pass-through.

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 1280×720 | `kDefaultWidth/Height` | `PlayModeLayout.h` | N |
| Outer padding | 12 | `kPresetBrowserPageOuterMargin = 12` | `PlayModeLayout.h`, `PresetBrowserOverlay.cpp` | N (2026-08-17) |
| Main workspace | 1256×576 | `kPresetBrowserPageMainWorkspaceHeight = 576` | `PlayModeLayout.h` | N (2026-08-17) |
| Column gap | 10 | `kPresetBrowserPageColumnGap = 10` | `PlayModeLayout.h` | N (2026-08-17) |
| Left categories | 240 | `kPresetBrowserLeftColumnWidth = 240` | `PlayModeLayout.h` | N (2026-08-17) |
| Center presets | 696 | `kPresetBrowserCenterColumnWidth = 696` | `PlayModeLayout.h` | N (2026-08-17) |
| Right detail | 300 | `kPresetBrowserRightColumnWidth = 300` | `PlayModeLayout.h` | N (2026-08-17) |
| Search field | 216×25 | `kPresetBrowserSearchFieldHeight = 25` | `PresetBrowserOverlay.cpp` | N (2026-08-17) |
| Category row | 25 | `kPresetBrowserCategoryRowHeight = 25` | `PlayModeLayout.h` | N (2026-08-17) |
| Preset row | 28 | `kPresetBrowserPresetRowHeight = 28` | `PlayModeLayout.h` | N (2026-08-17) |
| Table header | 18 | `kPresetBrowserTableHeaderHeight = 18` | `PlayModeLayout.h` | N (2026-08-17) |
| Detail waveform | 276×100 | `kPresetBrowserDetailWaveformHeight = 100` | `PlayModeLayout.h` | N (2026-08-17) |
| LOAD button | 276×33 | `kPresetBrowserLoadButtonHeight = 33` | `PlayModeLayout.h` | N (2026-08-17) |
| Footer bar | 40 | `kPresetBrowserFooterHeight = 40` | `PlayModeLayout.h` | N (2026-08-17) |
| ENGINES column | live engine indices | placeholder `01, 02, 04` | `PresetBrowserOverlay.cpp` | **Y** |
| AUTHOR column | preset metadata `"author"` | `PresetIndex` + `formatAuthorForEntry` | `PresetBrowserOverlay.cpp` | N (2026-08-17) |
| GRID view toggle | active UI | LIST + GRID; GRID scroll wired | `PresetBrowserOverlay.cpp` | N (2026-08-17) |
| EXPORT button | wired | display-only | `PresetBrowserOverlay.cpp` | **Y** |
| Faceted chip rows | absent in frame | removed from layout | — | N |

---

## Part 2i — `murmur-engine-deep-editor` (28:4) delta table

Figma layout at 1440×1024 (from `get_metadata` node `28:4`). Letterboxed into 1280×720 embed via `contentHost_` affine scale.

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 1440×1024 | `kEngineDeepEditorFrameWidth/Height` | `PlayModeLayout.h` | N (2026-08-17) |
| Outer margin | 24 | `kEngineDeepEditorOuterMargin = 24` | `PlayModeLayout.h` | N (2026-08-17) |
| Header bar | 64 | `kEngineDeepEditorHeaderHeight = 64` | `PlayModeLayout.h` | N (2026-08-17) |
| Engine tab pills | 8×28, gap 4 | `kEngineDeepEditorEngineTabHeight/Gap` | `EngineDetailOverlay.cpp` | N (2026-08-17) |
| ON/SOLO/MUTE | 34/46/47 × 19 | APVTS `MixEnabled/MixSolo/MixMute` | `EngineDetailOverlay.cpp` | N (2026-08-17) |
| Main content | 1392×832 | `kEngineDeepEditorMainContentHeight = 832` | `PlayModeLayout.h` | N (2026-08-17) |
| Column width | ~453.33 | `kEngineDeepEditorColumnWidth = 453` | `PlayModeLayout.h` | N (2026-08-17) |
| Column gap | 16 | `kEngineDeepEditorColumnGap = 16` | `PlayModeLayout.h` | N (2026-08-17) |
| OSC column | 453×625 | `OperatorEditorPanel` | `EngineDetailOverlay.cpp` | N (2026-08-17) |
| Filter column | 453×638 | `FilterLfoPanel` + `EngineAdsrMini` | `EngineDetailOverlay.cpp` | N (2026-08-17) |
| Amp column | 453×602 | painted level/unison/routing | `EngineDetailOverlay.cpp` | **Y** (display-only) |
| Bottom bar | 48 | `kEngineDeepEditorBottomBarHeight = 48` | `PlayModeLayout.h` | N (2026-08-17) |
| Footer CPU/voices | live | `PerformanceMetricsUi.h` | `EngineDetailOverlay.cpp` | N (2026-08-17) |
| Tab layout (OSC/FILTER/ENV) | absent — 3 columns | removed tab row | `EngineDetailOverlay.cpp` | N |

---

## Part 2a — `murmur-play-compact` (4:1134) delta table

Figma vertical budget at 320×565 (from `get_design_context` node `4:1134`):

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 320×565 | `kCompactWidth`, `kCompactDefaultHeight` | `PlayModeLayout.h`, `MurmurRootEditor.cpp` | N (2026-08-17) |
| Outer padding | 14 | `kCompactOuterMargin = 14` | `PlayModeLayout.h`, `MurmurRootEditor.cpp` | N (2026-08-17) |
| Section gap | 12 | `kCompactBlockGap = 12` | `PlayModeLayout.h`, `CompactModeEditor.cpp` | N (2026-08-17) |
| **Chrome: header-bar** | 32 (`39:158`) | `kChromeBarHeightCompact = 32` | `PlayModeLayout.h`, `MurmurRootEditor.cpp` | N (2026-08-17) |
| Mission / preset card | absent | removed from `CompactModeEditor` | `CompactModeEditor.cpp` | N (2026-08-17) |
| **Scope hub** | 152 (panel target) | `kCompactScopeSize = 152` | `PlayModeLayout.h`, `CompactModeEditor.cpp` | N (2026-08-17) |
| Macro grid knob | 36 | `kCompactMacroKnobSize = 36` | `PlayModeLayout.h`, `PatchFocusPanel.cpp` | N (2026-08-17) |
| Macro grid gap | 8 | `kCompactMacroKnobGap = 8` | `PlayModeLayout.h`, `PatchFocusPanel.cpp` | N (2026-08-17) |
| Master out knob | 48 | `kCompactMasterKnobSize = 48` | `PlayModeLayout.h`, `CompactModeEditor.cpp` | N (2026-08-17) |
| Output block height | 72 | `kCompactOutputBlockHeight = 72` | `PlayModeLayout.h`, `CompactModeEditor.cpp` | N (2026-08-17) |
| Footer mod chips / CPU | present in Figma | LFO1/ENV/SEQ/RAND chips + CPU/voices + stereo meters | `CompactModeEditor.cpp`, `OscilloscopeView.cpp` | N (2026-08-17 batch 3) |
| Scope panel style | rectangular + grid | `OscilloscopeView::paintCompactMode` | `OscilloscopeView.cpp` | N (2026-08-17 batch 3) |
| Stereo output meters | 6px bars in output block | `ScopeVuMeter` L/R in footer paint | `CompactModeEditor.cpp` | N (2026-08-17 batch 3) |

### Child tree (Figma reference)

```
murmur-play-compact (4:1134) 320×565
├── header-bar (39:158) 304×32 @ (14,14)  → MurmurChromeBar
├── scope-panel (4:1146)
├── performance-macros (4:1172)
├── output-control-block (4:1224)
└── footer-system (4:1245)
```

---

## Part 2b — `murmur-desktop-play-mode` (36:4) delta table

**Note:** This frame is **not** the same as `obsidian-play-board` (22:2). Basic PLAY content (scope, macro deck, bottom bar) lives in `PlayModeEditor`; chrome is unified `MurmurChromeBar` (`39:142`, 44px), not the legacy integrated `36:5` top-bar.

Figma vertical budget at 1280×720 (from `get_design_context` node `36:4`):

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame size | 1280×720 | `kDefaultWidth/Height` | `PlayModeLayout.h` | N |
| Outer padding | 20 all sides | `kDesktopPlayModeOuterMargin = 20` | `PlayModeLayout.h`, `MurmurRootEditor.cpp` | N (2026-08-17) |
| Section gap (major stacks) | 16 | `kDesktopPlayModeSectionGap = 16` | `PlayModeLayout.h`, `PlayModeEditor.cpp`, `MurmurRootEditor.cpp` | N (2026-08-17) |
| **Chrome: header-bar** | 44 (`39:142`) | `kChromeBarHeightPlay = 44` | `PlayModeLayout.h`, `MurmurRootEditor.cpp`, `MurmurChromeBar.cpp` | N (2026-08-17) |
| Chrome preset + tempo + master | performance-info (`39:154`) | preset name + BPM + `MASTER OUT` label | `MurmurChromeBar.cpp` | N (2026-08-17) |
| Legacy integrated top-bar | 72 (`36:5`) | `kDesktopPlayModeTopBarHeight = 72` (deprecated) | `PatchBrowserBar.cpp` | N — hidden |
| **Oscilloscope** | 180 | `kDesktopPlayModeOscilloscopeHeight = 180` | `PlayModeLayout.h`, `PlayModeEditor.cpp` | N (2026-08-17) |
| Scope metadata inset | 16, 12 | `kDesktopPlayModeScopeMetadataInsetX/Y` | `OscilloscopeView.cpp` | N (2026-08-17) |
| Scope wave padding X | 48 | `kDesktopPlayModeScopeWavePaddingX` | `OscilloscopeView.cpp` | N (2026-08-17) |
| Scope bar visualizer | 20× stereo bars | decorative metadata + waveform path | `OscilloscopeView.cpp` | **Y** (bars are mock; live path is waveform; CPU/VOICES wired via `PerformanceMetricsUi.h`) |
| **Performance macros deck** | 280 | `kDesktopPlayModePerformanceDeckHeight = 280` | `PlayModeLayout.h`, `PlayModeEditor.cpp` | N (2026-08-17) |
| Deck inner padding | 24 | `kDesktopPlayModePerformanceDeckPadding = 24` | `PatchFocusPanel.cpp` | N (2026-08-17) |
| Macros header | 13 | `kDesktopPlayModeMacrosHeaderHeight = 13` | `PatchFocusPanel.cpp` | N (2026-08-17) |
| Knobs row | 148 | `kDesktopPlayModeKnobsRowHeight = 148` | `PatchFocusPanel.cpp` | N (2026-08-17) |
| Macro knob cell | 120 (touch 92) | `kDesktopPlayModeMacroKnobWidth`, `MacroKnobTouchSize` | `PatchFocusPanel.cpp` | N (2026-08-17) |
| Macro knob gap | ~33 | `kDesktopPlayModeMacroKnobGap = 33` | `PatchFocusPanel.cpp` | N (2026-08-17) |
| Macro count | 8 | `kDesktopPlayModeMacroCount = 8` | `PatchFocusPanel.cpp` | N (2026-08-17) |
| **Bottom bar** | 64 (`36:226`) | `kDesktopPlayModeBottomBarHeight = 64` | `PlayModeLayout.h`, `VstBottomBar.cpp` | N (2026-08-17) |
| Bottom bar padding X | 24 | `kDesktopPlayModeBottomBarPaddingX = 24` | `VstBottomBar.cpp` | N (2026-08-17) |
| Engine indicators | 8×48, gap 6 | painted in `VstBottomBar::paint` | `VstBottomBar.cpp` | N (2026-08-17) |
| Latch / panic buttons | 127 / 101 × 33 | `kDesktopPlayModeLatchButtonWidth`, etc. | `VstBottomBar.cpp` | N (2026-08-17) |
| Sustain / MIDI / route | right cluster 235×24 | `sustainLabel_`, `midiLabel_`, `routeLabel_` | `VstBottomBar.cpp` | **Y** (labels only; latch not wired; view-mode strip hidden in Basic — double-click brand returns to Advanced) |

### Child tree (Figma metadata)

```
murmur-desktop-play-mode (36:4) 1280×720
├── header-bar (39:142) 1248×44 @ (20,20)  → MurmurChromeBar
├── oscilloscope (36:35) 1240×180 @ (20,80)
├── performance-macros-deck (36:155) 1240×280 @ (20,276)
└── vst-bottom-bar (36:226) 1240×64 @ (20,572)
```

**Code vertical budget (720 − 40 margin − 44 chrome − 16 gap = 620 content):** 180 + 16 + 280 + 16 + 64 = **556px** (64px flex slack).

---

## Part 2 — `obsidian-play-board` (22:2) delta table

Figma vertical budget at 1280×720 (from metadata):

| Region | Figma (px) | Code value | File:line | Fix? |
|--------|------------|------------|-----------|------|
| Frame outer padding | 16 all sides | `kOuterMargin = 16` | `PlayModeLayout.h:14` | N |
| Section gap (major stacks) | 12 | `kSectionGap = 12` | `PlayModeLayout.h:30` | N (2026-08-17) |
| **Chrome: vst-top-bar** | 54 (single bar inside frame) | `kObsidianChromeHeight = 54` unified row | `PlayModeLayout.h`, `MurmurRootEditor.cpp`, `PatchBrowserBar.cpp` | N (2026-08-17) |
| Preset browser width (in bar) | 320 | `kObsidianPresetBrowserWidth = 320` | `PlayModeLayout.h`, `PatchBrowserBar.cpp` | N (2026-08-17) |
| Master knob size (in bar) | 28 | `kObsidianMasterKnobSize = 28` | `PlayModeLayout.h`, `PatchBrowserBar.cpp` | N (2026-08-17) |
| **Engine grid region** | 402 total height | flex remainder after dashboard + bottom | `PlayModeEditor.cpp:587-593` | **Y** (derived) |
| Engine grid inner padding X | 16 (`engine-grid-placeholder` px) | `kEngineGridPadding = 16` | `EngineGridPanel.cpp:57` | N (2026-08-17) |
| Grid header row | 16 | `kEngineGridHeaderHeight = 16` | `PlayModeLayout.h`, `EngineGridPanel.cpp:58` | N (2026-08-17) |
| Grid cell gap (4×2) | 12 | `kEngineGridGap = 12` | `PlayModeLayout.h`, `EngineGridPanel.cpp:66` | N (2026-08-17) |
| Grid content padding Y | 16 (above/below card grid) | `kEngineGridContentPaddingY = 16` (`removeFromTop/Bottom` after header) | `PlayModeLayout.h`, `EngineGridPanel.cpp` | N (2026-08-17) |
| Engine card padding | 12 | `kEngineCardPadding = 12` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Engine card corner radius | 8 | `kEngineCardCornerRadius = 8` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Engine card row gap | 8 | `kEngineCardRowGap = 8` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Engine card header height | 14 | `kEngineCardHeaderHeight = 14` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| OSC picker size | 76×52 | `kEngineCardOscPickerWidth/Height` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| OSC type strip | 16 | `kEngineOscStripHeight = 16` | `PlayModeLayout.h`, `EngineOscillatorPicker.cpp` | N (2026-08-17) |
| OSC context grid cells | 36×14, gap 2 | `kEngineOscCellWidth/Height/Gap` | `PlayModeLayout.h`, `EngineOscillatorPicker.cpp` | N (2026-08-17) |
| Card knobs (pitch/filter) | 44×42, dial 28 | `kEngineCardKnobWidth/Height/DialSize` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Filter mode pill row | 16 (pill 12) | `kEngineCardFilterModeRowHeight` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| ADSR mini envelope group | 64×50 | `kEngineCardEnvelopeWidth/Height` | `PlayModeLayout.h`, `EngineAdsrMini.cpp` | N (2026-08-17) |
| Level row | 8 (track 6) | `kEngineCardLevelRowHeight/SliderHeight` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| **Play-board card stub (22:44)** | 86 total (62 content + 24 pad) | `kEngineCardPlayBoardHeight = 86` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Play-board card header | 12 | `kEngineCardPlayBoardHeaderHeight = 12` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Play-board middle row | 22 (osc stub 50 + knob dials 16) | `kEngineCardPlayBoardMiddleRowHeight` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Play-board row gap | 10 | `kEngineCardPlayBoardRowGap = 10` | `PlayModeLayout.h`, `EngineCard.cpp` | N (2026-08-17) |
| Play-board pitch knob stubs | decorative dials | live COARSE/FINE from operator APVTS | `EngineCard.cpp` | N (2026-08-17) |
| Full card (4:38) in grid | 180×339 (detail / 1440×1024 ref) | `EngineDetailOverlay` on double-click | `EngineDetailOverlay.cpp` | N (2026-08-17) |
| Polyphony badge width | 112 | `kPolyphonyBadgeWidth = 112` | `EngineGridPanel.cpp:60` | N (2026-08-17) |
| **Dashboard strip** | 168 | `kDashboardStripHeight = 168` | `PlayModeLayout.h`, `PlayModeEditor.cpp:591` | N (2026-08-17) |
| Dashboard FX : filter split | 830 : 406 (gap 12) | `kDashboardFilterPanelWidth=406`, gap 12 | `DashboardStrip.cpp:39` | N (2026-08-17) |
| Dashboard inner padding | 8 (fx-chain-flow) | `kDashboardInnerPadding = 8` | `DashboardStrip.cpp:38` | N (2026-08-17) |
| FX chain flow height | 52 | `kFxChainFlowHeight = 52` | `PlayModeLayout.h`, `FxChainStrip.cpp` | N (2026-08-17) |
| FX flow insert tap label | PRE/POST | `FxChainFlowView` reads `fxRoutingPrePost` | `FxChainFlowView.cpp` | N (2026-08-17) |
| FX slot editor height | 94 | `kFxSlotEditorHeight = 94` | `PlayModeLayout.h`, `FxChainStrip.cpp` | N (2026-08-17) |
| Global filter panel width | 406 | `kDashboardFilterPanelWidth = 406` | `DashboardStrip.cpp:39` | N (2026-08-17) |
| Global filter knob size | 36 | `kDashboardGlobalFilterKnobSize = 36` | `PlayModeLayout.h`, `FilterLfoPanel.cpp` | N (2026-08-17) |
| **Status / bottom bar** | 28 (`status-bar`) | `kVstBottomBarHeight = 28` | `PlayModeLayout.h:58`, `PlayModeEditor.cpp:589` | N |
| Bottom bar padding X | 16 | `kVstBottomBarPaddingX = 16` | `PlayModeLayout.h`, `VstBottomBar.cpp` | N (2026-08-17) |
| Lab launcher chips | VOCODER + LFO only | VOCODER + LFO (`setPlayBoardMode`) | `VstBottomBar.cpp` | N (2026-08-17) |
| Gap before bottom bar | 12 (flex gap) | `kSectionGap = 12` | `PlayModeEditor.cpp:590` | N (2026-08-17) |
| Default window size | 1280×720 | `kDefaultWidth/Height = 1280/720` | `PlayModeLayout.h:6-7` | N |

### ARP cross-check (`murmur-design-arp` 4:1267)

| Spec | Figma | Code | File:line | Fix? |
|------|-------|------|-----------|------|
| Frame size | 1280×720 | full overlay bounds | `ArpPanelOverlay.cpp` | N |
| Left panel width | 220 | `kArpSidePanelWidth = 220` | `PlayModeLayout.h` | N |
| Main workspace gap | 12 | `kDesignArpPageSectionGap = 12` | `PlayModeLayout.h` | N (2026-08-17) |
| Main workspace height | 558 | flex minus header/footer | `ArpPanelOverlay.cpp` | verify |
| Center column width | 784 | flex between 220 side panels | `PlayModeLayout.h` | N |
| Sequencer card height | 248 | flex top region | `kDesignArpPageSequencerCardHeight` | N |
| Pattern tuning height | 298 | `kArpBottomPanelHeight = 298` | `PlayModeLayout.h` | N (2026-08-17) |
| Design lab header | — (chrome only) | 56px `← DESIGN` when embedded | `kDesignLabPanelHeaderHeight` | Y (intentional) |
| PLAY overlay header | lab chrome | 36px + badge/title/ARP ON | `kArpHeaderHeight` | N |
| Bottom bar height | 44 | `kArpFooterHeight = 44` | `PlayModeLayout.h` | N |
| Step column width | ~43.25 (16 in 752px) | distributed remainder in `ArpStepStrip` | `ArpStepStrip.cpp` | N (2026-08-17) |
| Velocity lane | 32×130 | `kArpStepLaneWidth/Height` | `PlayModeLayout.h`, `ArpStepStrip.cpp` | N (2026-08-17) |
| Velocity curve card | 280×298 | `kArpVelocityCurveCardWidth/Height` | `PlayModeLayout.h` | N (2026-08-17) |
| Clock resolution grid | 150×52, buttons 74×18 | `kArpSyncGrid*` constants | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| Timing knobs | 64×55, dial 32 | `kArpTimingKnob*` | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| Routing row height | 33, toggle 22×12 | `kArpRoutingRowHeight` | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| Footer diagnostics | CPU / VOICES / MIDI IN | live CPU bar + %; active voices / 32; MIDI LED | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| Footer MOD chips | center strip LFO1..RAND | painted chips; LFO/ENV highlight from live routes | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| PATTERN HOLD lock | 14px lock + toggle row | hold row bg + lock glyph + APVTS latch toggle | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| CPU meter | decorative 40% | live `getCpuLoadPercent()` + active voice count | `ArpPanelOverlay.cpp` | N (2026-08-17) |
| Back button (design) | — | `← DESIGN` | `ArpPanelOverlay.cpp:262` | N (2026-08-17) |
| Back button (play) | — | `← PLAY BOARD` | `ArpPanelOverlay.cpp:262` | N |
| Swing knob | present | APVTS + `GlowKnob` | `ArpPanelOverlay.cpp` | N |
| Engine routing names | SINE LEAD, etc. | waveform/type + role suffix from patch | `refreshEngineRouting` | N (2026-08-17) |
| DN-UP mode | not in Figma | 7th mode button kept | `ArpPanelOverlay.cpp` | **Y** (engine) |

---

## Part 3 — Prior session completion check

| Item | Status | Evidence |
|------|--------|----------|
| `WavetableLabPanel` wired + CMake | **DONE** | `PlayModeEditor.h:138`, `PlayModeEditor.cpp:90-91`, `plugin/CMakeLists.txt:115` |
| ARP Swing APVTS (`kNumArpFields=9`) | **DONE** | `PluginState.h:81`, `PluginState.cpp:178-188` field 8 = Swing |
| `cycleArpStepRatchet` processor + click | **DONE** | `PatchworkEightProcessor.cpp:1207`, `ArpStepStrip.cpp:85` |
| Build verification | **PASS** | `cmake --build build --target pw8_plugin` exit 0 (2026-08-17 batch 5: SAT stubs, insert pre/post, play-board polish) |

---

## Part 4 — Spec-driven implementation checklist (for agents)

1. **Always call `get_design_context` with pinned node ID first**  
   Example: fileKey `PFt0LG6XmOiZWcSoUXIWIg`, nodeId `22:2`. Load `figma-design-to-code` skill before the call. Use `get_metadata` only to extract numeric bounds.

2. **Copy numbers literally into `PlayModeLayout.h`**  
   Do not round, scale, or “fit” unless the frame itself is a different size (e.g. 1440×1024 → document scale factor separately). One named constant per Figma dimension.

3. **No new layout invention**  
   If Figma shows a single 54px `vst-top-bar`, do not split into 64+30 without a matching Figma update. If code needs a row Figma lacks, flag it in this audit — do not silently add UI.

4. **Screenshot acceptance criteria**  
   - Export Figma frame PNG at 1× (1280×720).  
   - Run plugin advanced view at same size with factory preset matching mock (`OBSIDIAN SUB BASS`).  
   - Overlay diff: outer margin, grid gaps, dashboard height, bottom bar must align within **1px**.  
   - Lab overlays (ARP, vocoder, LFO, mod, wavetable): repeat at same frame size before marking DONE.

### Suggested implementation order

See [`OBSIDIAN_BUILD_MAP.md` — Recommended next work](OBSIDIAN_BUILD_MAP.md#recommended-next-work-dependency-first) for dependency-first ordering. Summary:

1. **Phase 2a unified header (code)** (`37:788`, `22:3`, `36:5`, `4:1134`) — wire `COMPACT | PLAY | DESIGN` + design sub-nav to swap views per [Figma Prototype Navigation Map](#figma-prototype-navigation-map). **Figma prototype: DONE.**  
2. **`obsidian-play-board` (22:2)** — highest-traffic Advanced PLAY; layout constants largely aligned.  
3. **Design sub-nav page routing (code)** — embed ARP (`4:1267`), vocoder (`15:4`), FX (`35:4`), MOD (`27:265`) under `DesignModeEditor` matching prototype links.  
4. **Per-osc context grids** (`28:921`, `28:1281`, `28:1605`) — 9 engine variants on `EngineOscillatorPicker`.  
5. **Lab overlay polish** — ARP footer 44px, vocoder signal diagram, mod matrix column widths.  
6. **`murmur-engine-deep-editor` (28:4)** — largest surface, 1440×1024.  
7. **`murmur-play-compact` (4:1134)** — margin/height delta vs Figma.

---

## Figma Prototype Navigation Map

Canonical tab → frame routing in Figma prototype mode (file `PFt0LG6XmOiZWcSoUXIWIg`). Active tab is excluded from self-navigation in prototype links.

### Top-level mode tabs

| Tab | Target frame | Node ID | Code owner (implementation target) |
|-----|--------------|---------|-----------------------------------|
| **COMPACT** | `murmur-play-compact` | `4:1134` | `CompactModeEditor` |
| **PLAY** (Basic) | `murmur-desktop-play-mode` | `36:4` | `PlayModeEditor` Basic |
| **PLAY** (Advanced) | `obsidian-play-board` | `22:2` | `PlayModeEditor` Advanced — adjunct surface; verify which PLAY tab variant prototype uses |
| **DESIGN** | `murmur-design-engine` | `37:787` | `DesignModeEditor` |

### Design sub-nav (within DESIGN mode)

| Tab | Target frame | Node ID | Code owner (implementation target) |
|-----|--------------|---------|-----------------------------------|
| **ENGINE** | `murmur-design-engine` | `37:787` | `DesignModeEditor`, `EngineGridPanel` |
| **ARP** | `murmur-arp-view` | `4:1267` | `ArpPanelOverlay`, `ArpStepStrip` |
| **VOCODER** | `murmur-vocoder-lab` | `15:4` | `VocoderLabPanel` |
| **FX** / **FX RACK** | `murmur-design-fx` | `35:4` | `DesignFxPanel`, `DesignFxSignalChain`, `FxChainStrip` |
| **FX chip (prototype)** | per-slot hero editor | see Part 2j | *not built* — `DesignFxDetailRouter` target |
| **MOD MATRIX** | `murmur-mod-matrix` | `27:265` | `DesignModMatrixPanel`, `ModRoutingOverlay` (PLAY) |
| **BROWSE** / **PRESET** | `murmur-preset-browser` | `27:6` | `PresetBrowserOverlay`, `PatchBrowserBar` |
| **WAVETABLE** *(lab)* | `murmur-wavetable-editor` | `27:709` | `WavetableLabPanel`, `WavetableStackView` |
| **DUAL LFO** *(lab)* | `murmur-dual-lfo-lab` | `15:247` | `DualLfoLabPanel` |

**Code gap:** ~~Implement the above routing in `DesignModeHeaderBar` / unified header~~ **DONE (2026-08-17)** — `MurmurChromeBar` + `DesignModeEditor` sub-page routing. Remaining: pixel polish per frame, standalone FX page layout (`35:4`), lab panel chrome/back-button copy for design context.

---

## Part 5 — Engine card vertical budget & clipping fix (2026-08-17)

### Vertical budget at 1280×720 (Advanced / obsidian-play-board)

| Step | px | Notes |
|------|-----|-------|
| Frame height | 720 | `kDefaultHeight` |
| − outer margin (×2) | −32 | `kOuterMargin = 16` |
| − chrome (`vst-top-bar` 22:3) | −54 | `kObsidianChromeHeight` |
| **Play editor content** | **634** | `MurmurRootEditor` → `PlayModeEditor` |
| − bottom bar (`status-bar` 22:414) | −28 | `kVstBottomBarHeight` |
| − section gap | −12 | `kSectionGap` |
| − dashboard strip (22:188) | −168 | `kDashboardStripHeight` |
| − section gap | −12 | `kSectionGap` |
| **Engine grid panel** | **414** | `EngineGridPanel` bounds |
| − grid padding (×2) | −32 | `kEngineGridPadding = 16` |
| − grid header (22:38) | −16 | `kEngineGridHeaderHeight` |
| − content padding top | −16 | `kEngineGridContentPaddingY` |
| − content padding bottom | −16 | `kEngineGridContentPaddingY` |
| **Card grid area** | **334** | 2 rows × 4 cols |
| − inter-row gap | −12 | `kEngineGridGap` |
| **Per-cell height** | **161** | `(334 − 12) / 2` |
| − card padding (×2) | −24 | `kEngineCardPadding = 12` |
| **Per-cell content height** | **137** | code stretches card to fill cell |

### Figma card anatomy comparison

| Node | Size | Rows | Authoritative for |
|------|------|------|-----------------|
| **22:44** play-board stub | 86×105 (grid slot ~171 tall) | header 12 + middle 22 + level 8, gaps 10, pad 12 | **obsidian-play-board grid** |
| **4:38** full engine card | 180×339 | header 14 + pitch 52 + filter/env 62 + level 8, gaps 8, pad 10–12 | **murmur-8-engine-vst**, detail overlay |

Full `4:38` content = **160px** (+ 24px padding = 184px card). Grid cells provide **137px** content → **~23px clip** on filter/level rows when rendering full card.

Option A (trim grid padding/header) could recover at most ~10px without breaking Figma grid spec (22:37 keeps 16px content padding). Still insufficient for 160px full card.

### Decision: **Option C** — `EngineCard::setPlayBoardCompactMode(true)`

Figma MCP metadata proves play-board cards (`22:44`) are **intentionally simplified stubs**, not scaled-down `4:38` cards. Implementation:

- `EngineGridPanel` enables compact mode on all grid cards.
- Compact layout matches 22:44: LED + title + engine-type badge, osc stub, decorative pitch knob dials, level row.
- Full card anatomy (`4:38` constants) remains available for future non-play-board surfaces; deep editing via `EngineDetailOverlay` (28:4) on double-click.

### Build verification

`cmake --build build --target pw8_plugin` — required after change.

---

## Appendix — MCP measurement sources

- `obsidian-play-board`: `get_metadata` + `get_design_context` node `22:2`  
- `murmur-design-engine`: `get_metadata` + `get_design_context` node `37:787`
- `murmur-play-compact`: `get_design_context` node `4:1134`  
- `murmur-desktop-play-mode`: `get_metadata` + `get_design_context` node `36:4`  
- `murmur-arp-view`: `get_metadata` + `get_design_context` node `4:1267`  
- Per-FX hero editors: `get_metadata` nodes `63:8`, `63:368`, `63:724`, `63:1090`, `63:1451`, `63:1836`, `63:2227`, `63:2590`, `63:2934`, `63:3309`  
- All other frames: `get_metadata` on listed node IDs  
- Code layout: `PlayModeLayout.h`, `MurmurRootEditor.cpp`, `PlayModeEditor.cpp`, `EngineGridPanel.cpp`, `DashboardStrip.cpp`, `VstBottomBar.cpp`
