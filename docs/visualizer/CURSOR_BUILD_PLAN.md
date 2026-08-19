# MURMUR 8 VISUALIZER — CPU BUILD PLAN

**Status:** GPU / OpenGL visualizer work is **scrapped**. Do not add new `OpenGLContext`, shader modules, or `MurmurVisualizerComponent` wiring for FX or wireframe previews. All product visuals use the CPU three-tier model documented in `docs/visualizer/README.md`.

Reference concept art lives in `docs/visualizer/concepts/` (waveform ribbon, chorus traces, reverb particles, delay rings). Implement these with JUCE `Graphics`, `VisualPreviewCache`, and `FxAnimationAtlas` — not GLSL.

---

## Non-negotiable architecture rules

1. **CPU-only previews** — `juce::Graphics`, cached polylines, atlas frames, optional 2× supersample via `PreviewSurface`.
2. **Audio thread never allocates, never locks, never blocks.** Bus writes are atomic stores into fixed-size arrays only.
3. **Push at block rate, not sample rate.**
4. **Cache by quantized param keys** — use `VisualPreviewCache` / `FxAnimationAtlas` keys; do not recompute full curves every frame unless animating phase.
5. **Downsample before push** — one min/max per scope column; FFT bins not raw spectra.

---

## Three tiers (implemented)

| Tier | Mechanism | FX / wireframe use |
|------|-----------|-------------------|
| 1 — Static curves | `VisualPreviewCache` | Saturation transfer, EQ curve, compressor transfer |
| 2 — Animated FX | `FxAnimationAtlas` (24 frames) | Tape drift base path; optional atlas for heavy FX |
| 3 — Live signal | `AudioVisualizerBus` + CPU paint | EQ analyzer bars, compressor GR/IN meters, scope ribbon |

**Layer model:** background grid → cached/animated data (`paintPolylineCurve`, atlas blit) → live overlay (meters, handles, readouts).

---

## Design FX hero (`DesignFxHeroViz`) — CPU only

All `murmur-fx-*` detail frames render through `DesignFxHeroViz::paint*()` methods. No GL fallback.

| Chip | Figma frame | CPU painter | Notes |
|------|-------------|-------------|-------|
| Saturation | `63:8` | `paintSaturationTransfer` | Unity dashed ref, model badge, drive readout |
| Chorus | `63:368` | `paintChorusSpatializer` | 3 phase-offset voice traces, L/R, rate/depth |
| Tape | `63:724` | `paintTapeDrift` | Atlas + flutter overlay, WOW/FLUTTER labels |
| Mood | `63:1090` | `paintMoodResponse` | Spectral curve, mode badge |
| Freq shift | `63:1451` | `paintFreqShiftBode` | Spiral + echo rings |
| Fractal | `63:1836` | `paintFractalCloud` | Clouds atlas + stream |
| Reverb | `63:2227` | `paintReverbDecay` | Decay envelope + glow particles |
| EQ | `63:2590` | `paintEqCurve` | Graph-first, analyzer, param strip |
| Compressor | `63:2934` | `paintCompressorDynamics` | Transfer + IN/GR meters |
| Limiter | `63:3309` | `paintLimiterCeiling` | Waveform + ceiling + PK |

Master catalog: `docs/FX_FIGMA_PIXEL_IMPORT.md`.

---

## Remaining CPU polish (FX sprint)

1. Frame-by-frame proportion pass vs Figma `63:*` screenshots (margins, meter widths, title strip).
2. Card browser mini-viz parity with hero motifs (`DesignFxCardBrowser::paintCardVis`).
3. Code Connect stubs per `murmur-fx-*` frame (layout JSON only; no GL).

---

## Other CPU surfaces (already wired)

- **Wireframe:** Filter, LFO, Envelope, Mod routing, FX play dashboard (`FxCpuPreview`)
- **Engine:** Master envelope, ADSR mini, oscilloscope, header spectrum (CPU paths when GL disabled)
- **Wavetable:** `WavetableMeshPaint` mesh + frame minis

---

## Explicitly out of scope (do not implement)

- New `Shaders/*.h` modules for FX or wireframe
- Per-component `MurmurVisualizerComponent` for `DesignFxHeroViz`
- `MURMUR_ENABLE_GPU_VISUALIZERS` enablement for Logic AU validation
- GPU instancing, `uTime` shader animation loops, texture upload render paths for FX previews

Legacy OpenGL scaffold under `plugin/src/ui/visualizer/MurmurVisualizerComponent.*` remains for optional scope/header components only; **FX Figma import does not depend on it.**
