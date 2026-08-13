# ADR: PLAY-mode wavetable warp surface (Week 3–5)

**Status:** Accepted (Aug 2026)  
**Context:** Schema v3 adds five wavetable warp APVTS fields per operator (`WtBend`, `WtAsymmetry`, `WtSyncRatio`, `WtSyncAmount`, `WtFormantShift`). Week 1–2 DSP ships **bend + asymmetry**; Week 4 adds **sync amount** DSP; Week 5 adds **formant shift** post-read emphasis.

## Decision

- **PLAY OSC page** exposes **four knobs** for Wavetable engine: WT Bend, WT Asym, **Sync Amt** (`WtSyncAmount`), and **Formant** (`WtFormantShift`).
- **Sync ratio** (`WtSyncRatio`) remains APVTS-only (automation/preset round-trip) but has **no PLAY knob**. Sync ratio defaults to 1.0; use DESIGN or preset authoring to set ratio > 1.
- **DESIGN Wavetable tab** will host the full warp panel (including sync ratio) in Week 7; until then, use external `tools/wavetable_builder` via the **Open Builder…** stub.

## Consequences

- Factory/demo presets can use bend/asym/sync-amt/formant for audible warp motion in PLAY.
- Presets saved with non-default sync ratio round-trip; ratio affects audio when sync amount > 0.
- Formant warp applies post-read via a two-peaking emphasis bank — complements static `formant-vowel-*.json` tables.
- See `docs/DESIGN_AND_WARPS_PLAN.md` Week 4–5 milestones for mod matrix + FX detail panels.
