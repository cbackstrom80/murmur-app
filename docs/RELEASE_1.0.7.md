# MURMUR 1.0.7 — Live Topology & UI Week 2

Ships **UI differentiation Week 2** (graph reunification), **decked KOINS polish**, and Week 8 exit polish bundled for Ben MVP.

## UI — Live Topology (signature moment)

- **Live Topology strip** in PLAY Basic and Advanced — compact 8-node graph with engine icons under KOINS / between operator chips and context strip
- **Tap strip → fullscreen graph overlay** — existing `AlgorithmGraphView` in a dimmed modal; node click syncs operator selection
- **Edge pulse on performance** — MW/EXP and KOINS activity pulse edges connected to the active operator
- **Graph selection ↔ operator chips** — unified highlight across strip, overlay, and `EngineNodeStrip`

## UI — Icon system & components

- **`EdgeIconGrid`** — monoline JUCE paths for all 7 edge types; wired into DESIGN graph edge rows and `AlgorithmGraphView` legend
- **`EngineNodeStrip`** — unified node strip (PLAY Advanced, DESIGN wavetable panel); replaces duplicated `NodeSelectorRow`
- **KOINS mission card** — procedural amber frame around Basic performance row; MW/EXP badges pulse when active

## Decked knobs (Week 1 carry)

- Concentric decked `GlowKnob` style with perceived depth in Basic KOINS layout

## Bugfixes (if not already in your installed pkg)

- Expression mod source clamp (1.0.6.1)
- Algorithm graph apply APVTS sync before load (1.0.6.1)

## Tests

- **227** ctest cases pass (golden preset SHA256 regression included)

## Install

Download **`MURMUR-1.0.7-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases). Double-click the installer, quit Logic, rescan AU if prompted.

## Deferred to Week 3+

- Compact view-mode icon (not third equal tab)
- DESIGN tab icons + Matrix wireframe preview embed
- Interstellar HUD browser badge
- Full icon atlas CMake pipeline
