# MURMUR 1.0.8 — UI Weeks 3–4 (IA + Wireframe)

Ships **information architecture pass** (Week 3) and **wireframe panel consistency** (Week 4) for Ben MVP.

## UI — Compact teleprompter (Week 3)

- **Performance HUD layout** — circular scope hub, 4 KOINS in cardinal orbit, preset mission card (name, category, play hint)
- **View-mode chrome reduction** — Basic | Advanced + compact dial icon (not a third equal text tab)
- **Minimal compact chrome** — volume lives in header; scope mode hidden in teleprompter

## UI — WireframePanel family (Week 4)

- **`WireframePanel`** — shared procedural frame (corner ticks, title dot, ~1.4px stroke, obsidian fill)
- Wrapped: Filter/LFO **scope**, **WavetableStackView** mesh, **DesignFxDetailPanel** params
- **DESIGN Matrix route preview** — procedural lines from source chips to destinations (MVP)

## UI — Polish

- **Interstellar HUD badge** — coordinate tick marks + "INTERSTELLAR" capsule in preset bar and browser rows
- **DESIGN tab icons** — monoline JUCE paths on Graph / Matrix / FX / Wavetable tabs

## Tests

- **227** ctest cases pass (golden preset SHA256 regression included)

## Install

Download **`MURMUR-1.0.8-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases). Double-click the installer, quit Logic, rescan AU if prompted.

## Deferred to Week 5+

- KOINS MW/EXP badge MIDI flash animation
- Full icon atlas CMake pipeline
- ENV/FILTER tab merge (deep IA cut)
