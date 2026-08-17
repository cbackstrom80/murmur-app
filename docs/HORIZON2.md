# Horizon 2 — Competitive parity (shipped scope)

**Branch:** `cursor/favorites-unison-stack-daw`  
**Context:** PLAY-only UI (Basic / Compact / Advanced), 933 factory presets with 1–3 feature macro KOINS.

## Shipped in v1.1.1 (GLOBAL panel + morph rollout)

| Item | Where |
|------|--------|
| **GLOBAL → QUASAR panel** | `GlobalPanel` — CHAIN / QUASAR / OUTPUT sub-tabs in Advanced PLAY |
| **Morph on all 75 Spatial presets** | `scripts/add_morph_koin_spatial_presets.py` |
| **Quasar paramOverrides in morph** | `MorphKoinExecutor.hpp` — angles, heights, room damping |
| **Sidechain polish** | Smoother RMS follower; AU connect hints in performance badge |
| **Meta-mod stub** | `docs/META_MOD_PLAN.md` |

## Shipped in v1.1.0 (Quasar Phase 3 + Morph + Sidechain)

| Priority | Item | Where |
|----------|------|--------|
| 1 | **Quasar Phase 3** — dedicated mod destinations, headphone/speaker mode, delay freeze, crossfeed/elevation HRIR-lite, expanded QUASAR FX knobs | `BinauralSpace.hpp`, `ModMatrixTypes.hpp`, `FxChainStrip`, `PluginState` |
| 2 | **Morph KOIN executor** — `morphPosition` APVTS, 2–4 keyframe lerp, PLAY morph knob | `MorphKoinExecutor.hpp`, `PatchFocusPanel`, `PatchworkEightProcessor` |
| 3 | **20 Spatial morph presets** — INTIMATE ↔ VOID morphKoin + dissemination | `content/presets/factory/Interstellar/Spatial/001–020` |
| 4 | **Sidechain follower MVP** — AU input bus, envelope → mod matrix, UI badge | `SidechainFollower.hpp`, `PatchworkEightProcessor`, `EXT_OSCILLATOR_AU_THEORY.md` |
| 5 | **MCP** — `set_spread_bundle`, Quasar dest IDs, sidechain source | `mcp_server/` |

## Shipped in prior pass (Horizon 2)

| Priority | Item | Where |
|----------|------|--------|
| 1 | **Spectrum analyzer on FILTER tab** — WAVE \| FFT toggle on Advanced PLAY FILTER scope (waveform + log-frequency FFT). Header scope unchanged. | `FilterPanelScopeView`, `FilterLfoPanel` |
| 2 | **KOINS Phase 2 polish** — preset bar / mission card / Basic KOINS hints from `macros[i].description`; MCP `set_macro_koin`. | `ModRoutingUi`, `PatchBrowserBar`, `CompactModeEditor`, `PatchFocusPanel`, `mcp_server/` |
| 3 | **Concentric dual knobs** — Sync Amt + Formant (OSC wavetable); LFO Rate + Phase (FILTER tab). | `OperatorEditorPanel`, `FilterLfoPanel` |
| 4 | **PoliMATHS Spread KOINS** — preset bar / mission card / Basic hints show `BLOOM → Filter, WT, …` from active `modRoutes` (`spreadSummaryForMacro`). | `MacroSpread.hpp`, `ModRoutingUi` |
| 5 | **Modulation Dissemination (MVP)** — per-note macro capture for Macro1–3 when `voiceSettings.macroDissemination: true`; held voices ignore live macro sweeps. | `Engine.cpp`, `Patch.hpp`, `PatchSerializer` |

## Horizon 3 (PoliMATHS follow-ups)

| Item | Notes |
|------|-------|
| **0-Coast CLS mod** | West Coast overtone/multiply/balance + cycling slope on Classic engine — see [`COAST_CLS_MOD.md`](COAST_CLS_MOD.md). **Aspirational; deferred.** |
| **EngineType::External (op 0)** | Sidechain follower ships; full EXT oscillator deferred — see `EXT_OSCILLATOR_AU_THEORY.md`. |
| **GLOBAL tab QUASAR sub-panel** | Shipped v1.1.1 — `GlobalPanel` |
| **Dissemination for Macro4–8** | MVP samples featured macros (0–2) only; CC-mapped macros stay live. |
| **Spread channel weights** | PoliMATHS per-channel attenuverter emulation for unison voices. |
| **Meta-mod (macro → mod route depth)** | Documented in `docs/META_MOD_PLAN.md`; executor deferred. |

## Deferred (and why)

| Item | Reason |
|------|--------|
| **Dual-filter serial/parallel routing** | Filter 2 DSP + UI already ship serial (Filter1 → Filter2). Parallel/topology routing needs schema v3 field, Voice signal-path refactor, and preset migration — larger than Horizon 2 MVP. |
| **Full Hydrasynth macro assign UI** | Requires destination-picker screen; mod matrix + MCP cover agent authoring today. |
| **`macroBundles` schema** | Optional Phase 2 polish; flat `modRoutes` + `set_macro_koin` sufficient for agents. |

## Verify in Logic Pro (v1.1.0)

1. Install AU v1.1.0 (`scripts/install_au_local.sh` or release pkg).
2. Load **Interstellar/Spatial/001-nebula-drift** — sweep **Morph** knob; spatial distances and SPACE macro should morph INTIMATE ↔ VOID.
3. **Advanced → FX → QUASAR** — Output Mode Headphone vs Speaker; Crossfeed; delay feedback ≥ 0.99 freezes tail.
4. **Sidechain (AU)** — route vocal bus to MURMUR sidechain; MOD route SIDECHAIN → target; performance badge shows level.
5. **MOD tab** — Quasar destination labels (QSR1 Dist, Room, etc.) readable on routes.

## Verify in Logic Pro (Quasar Phase 2)

1. Rebuild AU (`cmake --preset dev && cmake --build --preset dev`).
2. Load **Interstellar/001-cathedral-nebula** — master slot M3 = **QUASAR** (headphones).
3. Hold C3–C5 chord; sweep **SPACE** macro — spatial mix, room size, and tail should open.
4. **Advanced → FX** — select M3, TYPE = QUASAR; tweak Distance / Angle / Room / Delay knobs.
5. A/B vs chorus on M2 — Quasar should feel outside-the-head, not detuned widening.

## Verify in Logic Pro (Horizon 2 baseline)

1. Build and install AU (see `WEEK8_EXIT_CHECKLIST.md`).
2. Load **Interstellar/001-cathedral-nebula** — Basic view shows macro KOINS; preset bar hint shows spread lines (`BLOOM → Filter, WT, Formant, …`) and macro descriptions. **Dissemination** is on: each held note keeps its macro snapshot — sweep BLOOM before a chord, then hold; re-sweep mid-chord does not retune active voices.
3. **Advanced → FILTER** — bottom scope: tap **FFT** for live spectrum while holding a note; **WAVE** for oscilloscope.
4. **Advanced → OSC** (wavetable op) — concentric **Sync Amt / Formant** and **WT Bend / Asym** knobs.
5. **Compact** — mission card hint line follows macro descriptions.

## MCP

```python
set_macro_koin(patch_id, slot=0, name="BLOOM",
    description="Opens filter and wavetable motion.",
    destinations=[
        {"destination": "filter_cutoff", "amount": 18.0},
        {"destination": "operator_wavetable_position", "target_index": 0, "amount": 0.35},
    ])
```
