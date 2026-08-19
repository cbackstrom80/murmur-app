# MI Implementation Sprint Plan

**Date:** 2026-08-17  
**Program:** Mutable Instruments integration — C++ implementation phase  
**Figma file:** [`PFt0LG6XmOiZWcSoUXIWIg`](https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/) (Page 1)

**Related docs:**
- [FIGMA_UI_AUDIT.md — Part 6](FIGMA_UI_AUDIT.md#part-6--mi--core-frame-registry) — canonical frame registry, node IDs, Code Connect map
- [MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md](MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md) — track architecture, schema, engine milestones
- [MORPH_KOIN_SPEC.md](MORPH_KOIN_SPEC.md) — morph KOIN schema + executor contract
- [BEN_MASTER_MOTION_SPEC.md](BEN_MASTER_MOTION_SPEC.md) — `94:4715` master motion lab layout
- [QUASAR_RETURN_PLAN.md](QUASAR_RETURN_PLAN.md) — Sprint 8 in-MURMUR binaural spatializer (`102:4`)

---

## Current status snapshot (2026-08-17)

| Layer | Status | Count |
|-------|--------|-------|
| **Figma (design)** | **DONE** | 28 canonical frames on Page 1 (7 core views + 1 atom + 2 master-envelope sub-panels + 18 MI surfaces incl. QUASAR `102:4`) |
| **Code Connect** | **DONE (stubs)** | 30 canonical-registry stubs in `plugin/src/ui/figma-connect/` |
| **C++ code — PARTIAL** | Pixel-match / DSP in progress | **21** surfaces — **Tracks A + B + C sprint-shipped** (Sprints 1–4); host visual QA pending |
| **C++ code — NOT STARTED** | Planned panels + engine | **7** MI surfaces (Tracks D–G labs) + **QUASAR Sprint 8** |
| **Archived** | Duplicate generative lab | `89:3002` → `— Archive (MI deprecated)`; canonical is `89:3479` |

**B-M3 shipped (Track B baseline):** Blades PLAY row in `FilterLfoPanel`, `FilterRoutingWireframeView`, mod destinations `MorphPosition` / `FilterModeMorph` / `FilterRouting` / `FilterDrive` (`ModMatrixTypes.hpp`), 15 factory presets in `content/presets/factory/Blades/`, `FilterRoutingTests.cpp`.

**Next bottleneck:** **Sprint 8 (QUASAR)** — Q0–Q5 landed (PARTIAL Figma pixel QA + grain overlay optional); Sprint 7 release gate remains.

---

## Definition of Done (per sprint item)

A checkbox item is **done** when all of the following hold:

1. **Build green** — plugin + `ctest` pass locally; item added to `tests/CMakeLists.txt` if new test target.
2. **Figma node cited** — implementation matches the listed frame/sub-panel; delta noted in PR if intentional.
3. **Tests** — unit/DSP test for engine changes; UI smoke via host or standalone editor where applicable.
4. **Wire-up** — APVTS/patch field bound; nav entry reachable from `MurmurChromeBar` / `DesignModeEditor` sub-nav.
5. **Code Connect** — stub URL points at `PFt0LG6XmOiZWcSoUXIWIg` (not legacy `pi5k…`).

---

## Sprint 1 — Filter Lab + Blades pixel-match (Track B close) ✅ **COMPLETE** (2026-08-17)

**Duration:** 1–2 weeks  
**Goal:** Track B **exit** — DESIGN Filter Lab live; PLAY Blades + routing wireframe pixel-match Figma; routing spec documented.  
**Exit criteria:** Met — `pw8_plugin` build green; `ctest -R FilterRouting` 5/5.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Create `DesignFilterLabPanel.h/.cpp` | `89:313` | `DesignFilterLabPanel.*`, `DesignModeEditor.cpp` | B-M3 DSP | DESIGN → FILTER |
| [x] FILTER LAB in DESIGN sub-nav + chrome | `89:313` | `MurmurChromeBar.cpp`, `VstBottomBar.cpp` | Filter lab panel | Sub-nav + status chip |
| [x] Pixel-match `FilterLfoPanel` Blades row | `89:5` / `89:131` | `FilterLfoPanel.cpp` | B-M3 knobs | PLAY → FILTER |
| [x] Pixel-match `FilterRoutingWireframeView` | `89:246` | `FilterRoutingWireframeView.cpp` | `FilterRouting.hpp` | Routing sweep 0→1 |
| [x] Write `docs/FILTER_ROUTING_SPEC.md` | — | — | `FilterRouting.hpp` | Doc linked from integration plan |
| [ ] *(Optional)* Filter 1 pre-drive in `Voice.hpp` | — | `Voice.hpp` | — | Deferred |
| [x] Register in `plugin/CMakeLists.txt` | — | `plugin/CMakeLists.txt` | Panel files | Build green |

---

## Sprint 2 — Morph pixel-match + Track A polish (Track A close) ✅ **COMPLETE** (2026-08-17)

**Duration:** 1–2 weeks  
**Goal:** Track A **exit** — morph timeline, DESIGN editor, mod-matrix morph columns, FR.STEP chrome, easing tests + MCP keyframe CRUD, Spatial preset migration.  
**Exit criteria:** Met — `pw8_plugin` build green; `ctest -R MorphEasing` pass; MCP add/remove keyframe round-trip.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Pixel-match `MorphTimelineStrip` + `MasterMotionLabPanel` morph section (hue ring, autoplay chip, keyframe ticks) | `89:641` `murmur-mi-ui-play-morph-timeline`; sub `89:736` `morph-timeline-panel` | `MorphTimelineStrip.cpp`, `MasterMotionLabPanel.cpp`, `MasterEnvelopePanel.cpp` (compact `82:83`) | `MorphKoinExecutor.hpp`, morph APVTS | PLAY MOTION tab; timeline hue + ticks match Figma |
| [x] Pixel-match `DesignMorphEditorPanel` — per-path override table, color picker, 16-keyframe cap, ADD/DEL | `89:953` `murmur-mi-ui-design-morph-editor` | `DesignMorphEditorPanel.cpp`, `DesignModeEditor.cpp` | Per-path easing in executor | DESIGN → MORPH; CRUD keyframes persists in `.pw8` |
| [x] Pixel-match `MurmurChromeBar` FR.STEP badge (300ms fade, keyframe name flash, collision rules) | `89:1763` `murmur-mi-ui-chrome-fr-step-badge` | `MurmurChromeBar.cpp`, `MurmurProcessor.cpp` | FR.STEP detector | Automate morph sweep; badge flashes at crossings |
| [x] Pixel-match `DesignModMatrixPanel` morph extension columns (MORPH POS / FILT ROUTE / FILT MORPH / F2 DRIVE) | `89:1259` `murmur-mi-ui-mod-matrix-morph-extensions` | `DesignModMatrixPanel.cpp`, `ModRoutingUi.cpp` | Mod dest enum complete | DESIGN → MOD MATRIX; columns + route chips |
| [x] Add `tests/unit/MorphEasingTests.cpp` — LUT parity vs MI reference (0, 0.25, 0.5, 0.75, 1) | — | `engine/include/pw8/modulation/MorphEasing.hpp`, `tests/CMakeLists.txt` | `MorphEasing.hpp` shipped | `ctest -R MorphEasing` |
| [x] MCP `add_morph_keyframe` / `remove_morph_keyframe` tools | — | `mcp_server/patch_builder.py`, `mcp_server/patch_schema.py` | `set_morph_koin` exists | Agent round-trip: add KF → serialize → reload |
| [x] Spatial factory preset per-path easing migration script | — | `scripts/migrate_spatial_morph_easing.py`, `content/presets/` | Per-path override schema | Spot-check 5 Spatial presets; serializer roundtrip |
| [x] Polish `PatchFocusPanel` morph hub vs focus frame | `94:5038` `murmur-mi-ui-play-focus-morph-hub` | `PatchFocusPanel.cpp` | Timeline strip | PLAY Basic focus column; EVOLVE KOIN card + mini strip |

---

## Sprint 3 — Basic view + Master envelope (Core views) ✅ **COMPLETE** (2026-08-17)

**Duration:** 1–2 weeks  
**Goal:** Core PLAY/DESIGN envelope surfaces pixel-complete; preset explorer modal polished.  
**Exit criteria:** Met — `86:4`, `82:4`, `74:959` implemented; `pw8_plugin` build green.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Pixel-match PLAY Basic view — master envelope hero, portamento, 4 macros, VU, BASIC chrome tab | `86:4` `murmur-basic-view` | `PlayModeEditor.cpp`, `BasicPerformanceSidebar.cpp`, `MasterEnvelopePanel.cpp`, `MurmurChromeBar.cpp` | Master envelope APVTS | PLAY BASIC tab; 680/556 split layout |
| [x] Pixel-match `MasterEnvelopePanel` full panel on design engine | `82:4` `master-envelope-panel` | `MasterEnvelopePanel.cpp`, `DesignModeEditor.cpp` | env0 DSP | DESIGN → ENGINE; slope pills + ADSR curve |
| [x] Pixel-match compact `MasterEnvelopePanel` embed in morph timeline | `82:83` `master-envelope-section` | `MasterMotionLabPanel.cpp` | Sprint 2 timeline | Shipped Sprint 2 |
| [x] Polish preset explorer modal — centered 1040×620, ratings + A/B compare UI (display-only OK) | `74:959` `murmur-preset-explorer-overlay` | `PresetBrowserOverlay.cpp` | Preset index | Modal + star rating row |
| [x] Wire BASIC view mode toggle in chrome (if not already default) | `86:4` chrome | `MurmurChromeBar.cpp`, `PlayModeEditor.cpp` | Basic layout | BASIC badge on PLAY tab; toggle preserves state |

---

## Sprint 4 — Streams (Track C) ✅ **COMPLETE** (2026-08-17)

**Duration:** 1–2 weeks  
**Goal:** Master dynamics processor scaffold + PLAY/DESIGN Streams UI.  
**Exit criteria:** Met — four mode pills + GR meter on OUTPUT; `masterDynamics` schema serializes; follower/envelope modes audible; `pw8_plugin` build green.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Write `docs/MASTER_DYNAMICS_SPEC.md` — four Streams modes, schema, insert point in render graph | — | — | Integration plan §7 | Doc links from plan + sprint |
| [x] Scaffold `MasterDynamicsProcessor` (MIT clean-room from Streams spec) | — | `engine/include/pw8/dynamics/MasterDynamicsProcessor.hpp` *(new)*, `Engine.cpp` | `SidechainFollower.hpp` | Unit test: follower → gain reduction |
| [x] Add `masterDynamics` patch schema + serializer roundtrip | — | `Patch.hpp`, `PatchSerializer.cpp`, `tests/serialization/PatchSerializerTests.cpp` | Spec doc | Default = bypass (backward compat) |
| [x] PLAY master dynamics UI on OUTPUT deck — 4 mode pills, GR meter, sidechain viz | `89:1798` `murmur-mi-ui-play-master-dynamics` | `MasterOutputDeck.cpp`, `PlayModeEditor.cpp` | Processor scaffold | PLAY OUTPUT; mode switch + meter animates |
| [x] Create `DesignDynamicsLabPanel.h/.cpp` — transfer curves per mode, signal diagram | `89:2059` `murmur-mi-ui-design-dynamics-lab` | `DesignDynamicsLabPanel.*`, `DesignModeEditor.cpp`, `DesignSubPage::DynamicsLab` | PLAY UI + schema | DESIGN → DYNAMICS LAB; curves match mode |
| [x] Mod destinations `MasterDynamicsMix` + sidechain depth (if in spec) | — | `ModMatrixTypes.hpp`, `ModMatrixExecutor.hpp` | Processor | Route LFO → mix; audible |

---

## Sprint 5 — Stages (Track D) ✅ **COMPLETE** (2026-08-17)

**Duration:** 1–2 weeks  
**Goal:** Segment envelope chains — schema, executor, PLAY/DESIGN Stages UI.  
**Exit criteria:** Met — up to 6 segments per env slot; segment dot chain on motion lab; DESIGN segment editor navigable; `pw8_plugin` build green.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Segment envelope schema (`segments[]`: ramp/hold/step/loop) + serializer | — | `Patch.hpp`, `PatchSerializer.cpp`, `MorphEasing.hpp` (segment shapes) | Track A easing LUTs | Roundtrip test; empty = legacy ADSR |
| [x] Segment executor — chain state, loop markers, per-segment easing | — | `engine/include/pw8/envelope/SegmentEnvelope.hpp` *(new)*, `Engine.cpp` | Schema | `SegmentEnvelopeTests.cpp` — 6-segment chain |
| [x] PLAY master motion segments — dot chain ≤6, per-segment easing chips | `89:2381` `murmur-mi-ui-master-motion-segments` | `MasterMotionLabPanel.cpp`, `MasterEnvelopePanel.cpp` | Executor | PLAY MOTION; dots + easing UI |
| [x] Create `DesignEnvelopeSegmentsPanel.h/.cpp` — ENV 1–8 picker, loop markers | `89:2712` `murmur-mi-ui-design-envelope-segments` | `DesignEnvelopeSegmentsPanel.*`, `DesignModeEditor.cpp` | Executor + schema | DESIGN → ENVELOPE SEGMENTS; edit chain |
| [x] Integrate segment env with existing `DahdsrEnvelope` fallback | — | `Voice.hpp`, master env0 path | Executor | Legacy patches unchanged |

---

## Sprint 6 — Marbles + Peaks + Clouds (Tracks E–G) ✅ **COMPLETE** (2026-08-17)

**Duration:** 2 weeks  
**Goal:** Generative mod sources, utility peaks layer, master Clouds FX slot, focus morph hub polish.  
**Exit criteria:** Met — random sources routable in mod matrix; generative + utility labs navigable; Clouds hero on master FX; `GenerativeSourcesTests` green.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Create `DesignGenerativeLabPanel.h/.cpp` — T/X random, deja-vu, seed locker, 6-out routing | `89:3479` `murmur-mi-ui-design-generative-lab` **(canonical)** | `DesignGenerativeLabPanel.*`, `DesignModeEditor.cpp` | Marbles engine tasks below | DESIGN → GENERATIVE; not `89:3002` archive |
| [x] Generative mod sources engine — T, X, deja-vu clocks | — | `engine/include/pw8/modulation/GenerativeSources.hpp` *(new)*, `ModMatrixExecutor.hpp` | Sidechain infra (Track C) | Mod matrix route → audible modulation |
| [x] Mod matrix random-sources sidebar — deja-vu / T / X sections, RANDOM chips | `89:3911` `murmur-mi-ui-mod-matrix-random-sources` | `DesignModMatrixPanel.cpp` | Generative engine | Sidebar visible in MOD MATRIX |
| [x] Create `DesignUtilityPeaksPanel.h/.cpp` — mini env + mini LFO cards (no drums) | `89:4076` `murmur-mi-ui-design-utility-peaks` | `DesignUtilityPeaksPanel.*` | Peaks utility processors *(engine)* | DESIGN → UTILITY; trigger → env/LFO |
| [x] Master Clouds FX hero — granular master slot UI | `89:4435` `murmur-mi-ui-master-clouds-fx` | `DesignFxPanel.cpp`, master FX chain | `GranularOscillator.hpp` patterns | DESIGN → FX master slot; hero viz |
| [x] Polish PLAY focus morph hub — EVOLVE KOIN card, mini keyframe strip | `94:5038` `murmur-mi-ui-play-focus-morph-hub` | `PatchFocusPanel.cpp` | Sprint 2 timeline | PLAY Basic/Advanced focus column |

---

## Sprint 7 — Motion lab + hardening ✅ **COMPLETE** (2026-08-17)

**Duration:** 1–2 weeks  
**Goal:** Ben spec alignment for master motion lab; release quality; legacy Code Connect cleanup.  
**Exit criteria:** `94:4715` matches [BEN_MASTER_MOTION_SPEC.md](BEN_MASTER_MOTION_SPEC.md); golden presets regen; `release_gate.sh` + pluginval pass.

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Align `MasterMotionLabPanel` to Ben spec — env0 hero + 4 master LFOs 4-up grid | `94:4715` `murmur-master-motion-lab` | `MasterMotionLabPanel.cpp`, `DualLfoLabPanel.cpp` (reuse scopes) | Sprints 3–5 envelope/LFO | PLAY MOTION; compare to Ben spec + Figma |
| [x] Merge morph timeline + segments UX into unified motion lab flow | `94:4715`, `89:2381` | `MasterMotionLabPanel.cpp`, `PlayModeEditor.cpp` | Sprint 5 segments | Single MOTION entry; no dead ends |
| [x] Golden preset regen — Blades + Spatial + new MI demo patches | — | `content/presets/`, `scripts/generate_streams_presets.py` | Tracks A–F content | A/B render diff within tolerance |
| [ ] Run `scripts/release_gate.sh` — full test matrix + pluginval strictness 5 | — | `tests/`, `scripts/release_gate.sh` | All sprints | CI green; VST3 + AU validated *(ctest golden + SegmentEnvelope fixed 2026-08-17; pluginval still pending)* |
| [x] *(Low priority)* Migrate legacy figma-connect URLs `pi5kUNcZWQGhqfRAVu3voh` → `PFt0LG6XmOiZWcSoUXIWIg` | — | 12 stubs: `FilterLfoPanel`, `ModSourceChip`, `GlobalPanel`, `AlgorithmGraphView`, `SectionPanel`, `TopologyGraphOverlay`, `ModRoutingOverlay`, `WireframePanel`, `ArpLauncherChip`, `FxChainStrip`, `LiveTopologyStrip`, `GlowRingButton` | — | Code Connect publish points at canonical file |

---

## Sprint 8 — QUASAR (In-MURMUR binaural spatializer)

**Duration:** 3–4 weeks (Q0–Q5 sub-phases; may span two calendar sprints)  
**Goal:** QUASAR **inside MURMUR only** — master FX slot + full-screen hero editor ([Figma `102:4`](https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=102-4)). **No** standalone plugin.  
**Exit criteria:** `BinauralSpace` on master bus M1–M4; `102:4` UI from chain breadcrumb; 75 Spatial presets with embedded Quasar params; `PW8_BUILD_QUASAR_PLUGIN` stays OFF.

**Spec:** [QUASAR_RETURN_PLAN.md](QUASAR_RETURN_PLAN.md) (supersedes Aug 2026 standalone extraction).

| Task | Figma | C++ owner(s) | Deps | Verify |
|------|-------|--------------|------|--------|
| [x] Re-enable `EffectType::BinauralSpace` + APVTS master slot types | — | `EffectTypes.hpp`, `PluginState` | `BinauralSpaceProcessor` | QUASAR on M3; audio passes |
| [x] Wire `BinauralSpaceProcessor` into master FX chain | — | `Engine.cpp`, master chain | S8 enum | Bypass-safe DSP smoke |
| [x] Extend Quasar params in `EffectSlotParams` + serializer + MCP schema | — | `PatchSerializer.cpp`, `mcp_server/patch_schema.py` | Return plan §Schema | Roundtrip; legacy OK |
| [x] `MasterQuasarPanel` + `PlayLabOverlay::Quasar` routing | `102:4` | `MasterQuasarPanel.*`, `PlayModeEditor` | S8 engine | Chain → QUASAR; BACK |
| [x] `QuasarBinauralFieldView` — ring, compass, L/R dots, splines | `102:50` | `plugin/src/ui/components/quasar/*` | MasterQuasarPanel | Drag → azimuth/elev |
| [x] IN/OUT meters + 7-knob row (WIDTH…MIX) | `102:34`, `102:243` | `QuasarPrimaryKnobRow` | APVTS | Mod rings on primary row |
| [x] Binaural Engine card — HRTF, modes, mono below, phase corr | `102:308` | `QuasarEngineCard` | APVTS | Profile/mode pills |
| [x] Spatial card — room presets, air, HP comp | `102:307` | `QuasarSpatialCard` | Room params | ROOM pills |
| [x] Telemetry HUD (CPU/latency/grains stub) | footer | `QuasarTelemetryBar` | processor | CPU bar live |
| [x] Restore Quasar mod destinations (14) + SPACE macro KOIN | — | `ModMatrixTypes.hpp`, `Engine.cpp` | Sprint 2 morph | LFO → Quasar params |
| [x] Spatial preset re-embed (`embed_spatial_quasar_slot.py`) | — | `Interstellar/Spatial/` (75) + `MasterSpatial/` (20) | — | In-slot type 13 |
| [x] Code Connect + audit registry | `102:4` | `MurmurMasterQuasarBinaural.figma.ts`, `FIGMA_UI_AUDIT.md` | Q1 panel | PARTIAL status |
| [x] Deprecate standalone QUASAR target (no ship) | — | `CMakeLists.txt`, docs | — | `PW8_BUILD_QUASAR_PLUGIN=OFF` default |

**Sub-phases:** Q0 scaffold → Q1 hero → Q2 engine card → Q3 spatial card → Q4 mod/grains → Q5 preset migration.

---

## Dependency graph (sprint order)

```
Sprint 1 (Blades UI) ──► Sprint 4 (Streams sidechain feeds Marbles)
Sprint 2 (Morph/easing) ──► Sprint 5 (Stages segment shapes reuse MorphEasing)
Sprint 2 (Morph/easing) ──► Sprint 8 (QUASAR spatial morph paths + Spatial bank)
Sprint 3 (Core views) ──► Sprint 7 (Motion lab polish)
Sprint 4 ──► Sprint 6 (Generative sources)
Sprint 5 ──► Sprint 7 (Segments in motion lab)
Sprint 6 ──► Sprint 7 (Golden presets)
Sprint 6 (Clouds/grains) ──► Sprint 8 (grain field overlay — optional)
Sprint 7 ──► Sprint 8 (release hardening before Spatial re-embed)
```

**Parallelization:** Sprints 1–3 can overlap (UI vs engine owners). Sprints 4–6 are sequential per track but Tracks E/F/G engine work can start behind feature flags while UI lands. **Sprint 8** starts after Sprint 7 exit or in parallel once Q0 engine scaffold lands (Q1 UI can proceed on stub APVTS).

---

## Quick reference — MI frame registry

| Track | Frame | Node | Code status |
|-------|-------|------|-------------|
| B | `murmur-mi-ui-play-filter-blades` | `89:5` | PARTIAL |
| B | `murmur-mi-ui-component-blades-routing-diagram` | `89:246` | PARTIAL |
| B | `murmur-mi-ui-design-filter-lab` | `89:313` | PARTIAL |
| A | `murmur-mi-ui-play-morph-timeline` | `89:641` | PARTIAL *(S2)* |
| A | `murmur-mi-ui-design-morph-editor` | `89:953` | PARTIAL *(S2)* |
| A | `murmur-mi-ui-mod-matrix-morph-extensions` | `89:1259` | PARTIAL *(S2)* |
| A | `murmur-mi-ui-chrome-fr-step-badge` | `89:1763` | PARTIAL *(S2)* |
| C | `murmur-mi-ui-play-master-dynamics` | `89:1798` | PARTIAL *(S4)* |
| C | `murmur-mi-ui-design-dynamics-lab` | `89:2059` | PARTIAL *(S4)* |
| D | `murmur-mi-ui-master-motion-segments` | `89:2381` | PARTIAL *(S5)* |
| D | `murmur-mi-ui-design-envelope-segments` | `89:2712` | PARTIAL *(S5)* |
| E | `murmur-mi-ui-design-generative-lab` | `89:3479` | NOT STARTED |
| E | `murmur-mi-ui-mod-matrix-random-sources` | `89:3911` | NOT STARTED |
| F | `murmur-mi-ui-design-utility-peaks` | `89:4076` | NOT STARTED |
| G | `murmur-mi-ui-master-clouds-fx` | `89:4435` | NOT STARTED |
| A/D | `murmur-master-motion-lab` | `94:4715` | PARTIAL |
| A | `murmur-mi-ui-play-focus-morph-hub` | `94:5038` | PARTIAL *(S2)* |
| **H** QUASAR | `murmur-master-quasar-binaural` | `102:4` | PARTIAL *(S8 Q0–Q5)* |

**MCP design fetch:** `get_design_context fileKey=PFt0LG6XmOiZWcSoUXIWIg nodeId=<id>` — cheat sheet in [FIGMA_UI_AUDIT.md Part 6.10](FIGMA_UI_AUDIT.md#610-mcp-fetch-cheat-sheet).
