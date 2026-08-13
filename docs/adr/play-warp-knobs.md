# ADR: PLAY-mode wavetable warp surface (Week 3)

**Status:** Accepted (Aug 2026)  
**Context:** Schema v3 adds five wavetable warp APVTS fields per operator (`WtBend`, `WtAsymmetry`, `WtSyncRatio`, `WtSyncAmount`, `WtFormantShift`). Week 1–2 DSP ships **bend + asymmetry** only (`WavetableWarp.hpp`).

## Decision

- **PLAY OSC page** exposes **two knobs only:** WT Bend and WT Asym (`OperatorEditorPanel`).
- **Sync ratio, sync amount, and formant shift** remain in APVTS for automation/preset round-trip but are **no-ops** until Week 4–5 warp DSP lands. They are **not** shown on PLAY.
- **DESIGN Wavetable tab** will host the full warp panel (including sync ratio) in Week 7; until then, use external `tools/wavetable_builder` via the **Open Builder…** stub.

## Consequences

- Factory/demo presets should rely on bend/asym for audible warp motion in Week 3 acceptance.
- Presets saved with non-default sync/formant values will round-trip silently with no audible effect until those warps ship.
- See `docs/DESIGN_AND_WARPS_PLAN.md` Week 3 status for sprint closure.
