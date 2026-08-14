# Horizon 2 — Competitive parity (shipped scope)

**Branch:** `cursor/favorites-unison-stack-daw`  
**Context:** PLAY-only UI (Basic / Compact / Advanced), 933 factory presets with 1–3 feature macro KOINS.

## Shipped in this pass

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
| **Dissemination for Macro4–8** | MVP samples featured macros (0–2) only; CC-mapped macros stay live. |
| **Spread channel weights** | PoliMATHS per-channel attenuverter emulation for unison voices. |
| **Agent `set_spread_bundle` MCP tool** | Optional channelWeight per destination. |

## Deferred (and why)

| Item | Reason |
|------|--------|
| **Dual-filter serial/parallel routing** | Filter 2 DSP + UI already ship serial (Filter1 → Filter2). Parallel/topology routing needs schema v3 field, Voice signal-path refactor, and preset migration — larger than Horizon 2 MVP. |
| **Full Hydrasynth macro assign UI** | Requires destination-picker screen; mod matrix + MCP cover agent authoring today. |
| **Meta-mod (macro → mod route depth)** | Engine gap documented in `ASM_MACRO_KOINS_RESEARCH.md`; not in `ModMatrixExecutor`. |
| **`macroBundles` schema** | Optional Phase 2 polish; flat `modRoutes` + `set_macro_koin` sufficient for agents. |

| 5 | **Global Quasar FX Phase 2** — `BinauralSpace` master slot with ITD/ILD panner, dual embedded `RoomEngine` paths, CNTR anchor, post-sum delay; 20 APVTS params; PLAY QUASAR UI; SPACE macro on Interstellar showcase presets. | `BinauralSpace.hpp`, `RoomEngine.hpp`, `BinauralPanner.hpp`, `FxChainStrip`, `EffectsTests.cpp` |

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
