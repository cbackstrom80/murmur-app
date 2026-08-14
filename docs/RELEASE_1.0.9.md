# MURMUR 1.0.9 — Consolidated Mod Matrix & dual concentric controls

Ships the **unified MOD MATRIX screen**, **deluxe matrix row styling**, **dual concentric performance knobs**, and bundles **1.0.7–1.0.8 UI** polish for Ben MVP.

## UI — Consolidated Mod Matrix (PLAY + DESIGN)

- **Single Mod Matrix screen** — reference-style routing table plus modulator context in one place (PLAY **MOD** tab and DESIGN Matrix)
- **`ObsidianMatrixLookAndFeel` + `ModMatrixRow`** — deluxe row chrome: scope chips, destination labels, depth readouts aligned with OBSIDIAN palette
- **DESIGN Matrix route preview** — procedural wireframe lines from source chips to destinations (carried from 1.0.8)

## UI — Dual concentric knobs (two params, one dial)

- **`ConcentricGlowKnob` LAF pass** — outer ring + inner hub map to **two related parameters** on one control
- **Filter panel** — outer/inner = **Cutoff / Resonance**
- **Wavetable warp** — outer/inner = **Bend / Asym**
- Functional drag semantics: independent outer vs inner adjustment without leaving PLAY performance layout

## UI — Weeks 3–4 (1.0.8 carry)

- **Compact teleprompter** — circular scope hub, KOINS in cardinal orbit, preset mission card; **◎** compact mode (not a third equal tab)
- **`WireframePanel` family** — shared procedural frames on filter/LFO scope, wavetable mesh, DESIGN FX detail
- **Interstellar HUD badge** — coordinate ticks + **INTERSTELLAR** capsule in preset bar and browser rows
- **DESIGN tab icons** — monoline paths on Graph / Matrix / FX / Wavetable tabs

## UI — Live Topology & KOINS (1.0.7 carry)

- **Live Topology strip** — 8-node graph under KOINS; tap → fullscreen `AlgorithmGraphView` overlay
- **Edge pulse** on MW/EXP and KOINS activity; selection sync with operator chips
- **Decked KOINS** — concentric `GlowKnob` depth styling, MW/EXP badges, mission-card frame

## Sound & content (prior releases in this lineage)

- **900 factory presets** — 800 core + **100 Interstellar** cinematic bank
- **Week 8 engine polish** — tempo-sync mod targets, Interstellar golden regression, pluginval soak fixes
- **1.0.6.1 fixes** — expression mod clamp, graph Apply APVTS sync, Interstellar pkg upgrade merge

## Tests

- Full **`ctest --preset dev`** suite (golden preset SHA256 regression included)

## Install

Download **`MURMUR-1.0.9-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.0.9). Double-click the installer, quit Logic, rescan AU if prompted.

Optional: **`MURMUR-1.0.9-macOS-arm64.dmg`** — same pkg inside a drag-and-install wrapper.

## Deferred

- KOINS MW/EXP badge MIDI flash animation
- Full icon atlas CMake pipeline
- ENV/FILTER tab merge (deep IA cut)
