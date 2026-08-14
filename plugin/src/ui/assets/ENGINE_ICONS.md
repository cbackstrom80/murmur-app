# Engine Icon Atlas — Week 1 Spec

Monoline 16×16 (24×24 @2x) icons for the eight Layer A engine types.
Style: single-stroke wireframe, matches Obsidian mesh family — not skeuomorphic.

| Engine | Icon metaphor | Stroke |
|--------|----------------|--------|
| Classic | Sine wave | Cyan (`kAccent`) |
| Wavetable | Three horizontal scan lines | Cyan |
| FM | Two overlapping circles (modulator/carrier) | Cyan |
| Additive | Four vertical partial bars | Cyan |
| PhaseShape | Triangle/ramp wave | Cyan |
| Granular | Three grain dots + arc | Cyan |
| Resonator | Bell / resonant peak curve | Cyan |
| Noise | Jagged noise burst | Cyan |

Performance-touched contexts (GLOBAL pill, macro row): amber stroke optional.
DESIGN authoring chrome: violet reserved for mode chrome, not engine icons.

Implementation: `EngineIconGrid.h/cpp` — JUCE `Path` icons, no raster atlas yet.
Week 2+: promote to SVG-derived `@2x` PNG atlas in CMake binary data.
