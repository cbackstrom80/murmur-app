# MURMUR 1.0.5 — Weeks 4–7: warp suite, DESIGN FX/mod UX, Interstellar presets

Ships **Weeks 4–7** milestones: expanded wavetable warps (sync, formant, mod destinations), DESIGN mod matrix and FX detail panels, graph/engine pill sync, golden warp render tests, Week 7 wavetable warp panel + DAW soak checklist, and the **Interstellar** factory bank (**900 presets** total).

## New in 1.0.5

### Week 4 — Sync warp, mod matrix DESIGN, WT Bend mod dest
- **Sync warp** mode in the wavetable warp engine with DSP tests
- **Sync Amt** control wired through voice/processor state
- **`ModMatrixDesignPanel`** — DESIGN-mode mod matrix editing aligned with routing UI
- **Wavetable Bend** as a modulation destination in the mod matrix executor and patch serialization
- PLAY warp knobs ADR and related UI/engine updates

### Week 5 — Formant warp, DESIGN FX detail panels
- **Formant warp** in `WavetableWarp` with unit tests
- **`DesignFxDetailPanel`** — Reverb, EQ, and Chorus detail editing in DESIGN
- **Formant factory presets** demonstrating formant warp tones

### Week 6 — Warp mod dests, graph/engine sync, golden hashes
- **Complete warp modulation destinations** across the mod matrix
- **Graph ↔ engine pill sync** so DESIGN graph state matches engine operator pills
- **Golden warp render hashes** for regression-safe DSP output

### Week 7 — Wavetable warp panel, validation prep, soak checklist
- **`WavetableWarpPanel`** in DESIGN for warp mode and amount editing
- **`scripts/run_pluginval.sh`** — pluginval runner for AU/VST3 validation prep
- **`docs/WEEK7_DAW_SOAK_CHECKLIST.md`** — manual DAW soak checklist
- **`PatchSerializer` mod-destination fix** for reliable preset round-trip

### Interstellar — 100 cinematic factory presets
- **100 new presets** in the **Interstellar** category (cosmic pads, gravity subs, pulsar clocks, leads, drones, FX, keys, bells, dual-layer stacks, wildcards)
- Generation script, README highlight reel, browser category **interstellar**, and load tests
- **900 factory presets** total (800 core + 100 Interstellar)

- Version bump 1.0.4 → 1.0.5

## Install

Download **`MURMUR-1.0.5-macOS-arm64.pkg`** (or `.dmg`) — same Ben MVP flow: double-click, quit Logic, rescan AU, play.

## Unchanged from 1.0.4

- Week 3: PLAY topology refresh after graph apply, warp/template factory presets, schema v3 QA, D5 PLAY warp knobs (Bend/Asym), DESIGN Wavetable Open Builder stub
- Horizon 1 (Filter 2, scope, mod UX), DESIGN shell, algorithm graph editor
- MURMUR Audio Unit, wavetable library, MP11SE + Logic docs, **PRODUCT_GAP_PLAN.md**
