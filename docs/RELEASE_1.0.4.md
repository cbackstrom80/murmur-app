# MURMUR 1.0.4 — Week 3: graph refresh, factory presets, schema v3 QA

Ships **Week 3** milestones: PLAY topology refresh after graph apply, new factory presets, schema v3 test coverage, and PLAY warp knob scope (D5).

## New in 1.0.4

### Graph Apply → PLAY topology refresh
- After applying the algorithm graph in DESIGN, **PLAY mode** refreshes operator topology so routing matches the committed graph.

### Factory presets
- **`Warp/warp-bend-demo.pw8`** — demo preset highlighting wavetable bend warp
- **`Templates/feedback-bell.pw8`** — template preset for feedback-style bell tones

### Schema v3 QA & compile-gate tests
- Serialization and factory preset load tests for **schema v3**
- **Algorithm graph commit** unit tests and compile-gate coverage

### D5 — PLAY warp knobs
- In **PLAY**, wavetable warp exposes **Bend** and **Asym** only (per D5 decision)

### DESIGN — Wavetable tab
- **Open Builder** stub on the DESIGN Wavetable tab for future builder workflow

- Version bump 1.0.3 → 1.0.4

## Install

Download **`MURMUR-1.0.4-macOS-arm64.pkg`** — same Ben MVP flow: double-click, quit Logic, rescan AU, play.

## Unchanged from 1.0.3

- Horizon 1 (Filter 2, scope, mod UX), DESIGN shell, algorithm graph editor, bend/asym warps
- MURMUR Audio Unit, factory presets, wavetables
- Full product doc set (`Docs/product/`), Logic + Kawai MP11SE guides, **PRODUCT_GAP_PLAN.md**
