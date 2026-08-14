# MURMUR 1.1.0 — Quasar Phase 3, Morph KOIN, Sidechain MVP

Ships **Global Quasar FX Phase 3 polish**, **runtime Morph KOIN executor**, **AU sidechain envelope follower**, and **20 Interstellar Spatial morph presets**.

## Sound — Quasar Phase 3

- **Dedicated Quasar mod destinations** — QSR1/2 distance, angle, height, room, delay feedback/time, CNTR level (not only MasterReverb* aliases)
- **Headphone vs speaker compensation** — `QuasarOutputMode` APVTS + ITD scale / crossfeed in `BinauralPanner`
- **Delay freeze** — feedback ≥ 0.99 holds tail (Quasar FW 2.0 style)
- **PLAY Advanced FX** — expanded QUASAR knobs: Output Mode, Crossfeed, CNTR, Q2 Dist, Room, Delay Fdbk
- **HRIR-lite** — elevation tilt + crossfeed refinement for out-of-head imaging

## Morph KOIN (Horizon 3 MVP)

- **`morphPosition` APVTS** — runtime lerp across 2–4 keyframes (`macroValues` + `paramOverrides`)
- **PLAY morph knob** — `uiFocus kind:morph` in PatchFocusPanel
- **20 Interstellar Spatial presets** — INTIMATE ↔ STAGE ↔ VOID morphKoin on Quasar/spatial params
- **MCP** — `set_morph_koin` drives live morph; `set_spread_bundle` alias for macro KOINS

## Sidechain follower (EXT path MVP)

- **AU sidechain input bus** — stereo `"Sidechain"` bus (Logic sidechain picker)
- **Envelope follower** → `ModSource::Sidechain` in mod matrix
- **Performance badge** — Sidechain (AU) indicator when routed or active

## Tests

- Full **`ctest --preset dev`** — 244 tests (243 run + 1 skipped golden regenerate)
- New: `MorphKoinTests`, Sidechain mod route, Quasar delay freeze

## Install

Download **`MURMUR-1.1.0-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.1.0). Double-click the installer, quit Logic, rescan AU if prompted.

Optional: **`MURMUR-1.1.0-macOS-arm64.dmg`**

## Deferred (see HORIZON2.md)

- Full `EngineType::External` on operator 0
- GLOBAL tab sub-panel (Quasar params live under Advanced → FX today)
- Meta-mod (macro → route depth)
