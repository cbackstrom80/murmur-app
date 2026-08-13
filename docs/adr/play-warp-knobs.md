# ADR: PLAY-mode wavetable warp surface (Week 3–4)

**Status:** Accepted (Aug 2026)  
**Context:** Schema v3 adds five wavetable warp APVTS fields per operator (`WtBend`, `WtAsymmetry`, `WtSyncRatio`, `WtSyncAmount`, `WtFormantShift`). Week 1–2 DSP ships **bend + asymmetry**; Week 4 adds **sync amount** DSP.

## Decision

- **PLAY OSC page** exposes **three knobs** for Wavetable engine: WT Bend, WT Asym, and **Sync Amt** (`WtSyncAmount`).
- **Sync ratio** (`WtSyncRatio`) and **formant shift** (`WtFormantShift`) remain APVTS-only (automation/preset round-trip) but have **no PLAY knob**. Sync ratio defaults to 1.0; use DESIGN or preset authoring to set ratio > 1.
- **DESIGN Wavetable tab** will host the full warp panel (including sync ratio) in Week 7; until then, use external `tools/wavetable_builder` via the **Open Builder…** stub.

## Consequences

- Factory/demo presets can use bend/asym/sync-amt for audible warp motion in PLAY.
- Presets saved with non-default sync ratio or formant values round-trip; ratio affects audio when sync amount > 0.
- See `docs/DESIGN_AND_WARPS_PLAN.md` Week 4 milestone for mod matrix + WT Bend mod dest.
