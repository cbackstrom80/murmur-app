# Murmur 8 CPU Visualizer Architecture

All wireframe and Design FX hero previews render on the **CPU** using JUCE `Graphics`. GPU / OpenGL visualizer work is **scrapped from the product plan** (see `CURSOR_BUILD_PLAN.md`).

## Tiers

| Tier | Mechanism | Examples |
|------|-----------|----------|
| 1 — Static curves | `VisualPreviewCache` polyline LUT (256 entries, quantized param keys) | Filter, LFO, ADSR, EQ, saturation, compressor transfer |
| 2 — Animated FX | `FxAnimationAtlas` runtime sprite bake (24 frames) | Chorus, tape drift, reverb decay, clouds/granular |
| 3 — Live signal | `AudioVisualizerBus` | Scope ribbon, FFT bars, envelope playhead, GR meters |

**Layer model:** background grid → cached data (optional 2× via `PreviewSurface` / `renderPlotHiRes`) → live overlay.

## Key files

```
plugin/src/ui/visualizer/
  VisualPreviewCache.*     — polyline cache
  PreviewDraw.h            — paintPolylineCurve, bus waveform/spectrum helpers
  PreviewSurface.*         — layered 2× bake (background + data + overlay)
  FxAnimationAtlas.*       — horizontal atlas bake + paintChorus/Tape/Reverb/Clouds
  FxCpuPreview.*           — all 14 FX wireframe previews
  AudioVisualizerBus.h       — lock-free audio → GUI snapshots
```

`VisualizerGpu.h` / `MurmurVisualizerComponent.*` exist as legacy optional paths for a few scope/header widgets when explicitly enabled; **Design FX hero and Figma import do not use them.**

## Wired components

- **Wireframe:** Filter, LFO, Envelope, Mod routing, FX (via `FxCpuPreview`)
- **Engine:** Master envelope, ADSR mini, Obsidian envelope, oscilloscope, header spectrum
- **Wavetable:** mesh sample grid + frame minis (`WavetableMeshPaint`)
- **Design FX hero:** `DesignFxHeroViz` — CPU painters only (cache + atlas + overlays)

## Offline atlas bake (optional)

```bash
python3 tools/visual_bake/bake_fx_atlases.py --out plugin/resources/fx_atlases
```

PNG atlases are embedded in `pw8_branding_data` BinaryData. `FxEmbeddedAtlases` loads them at startup; `FxAnimationAtlas` scales embedded frames for default param keys, runtime-bakes everything else.

## Theme-aware caches

`VisualTheme.h` fingerprints `palette::` accent colours into every polyline/atlas cache key. `PreviewSurface` and `VisualPreviewCache` auto-invalidate when the palette changes (e.g. after editing `ObsidianPalette.h`).

## Visual identity

Same glow stroke (`strokeGlowPath`), teal/warm palette, and grid treatment across all CPU previews so the instrument reads as one dashboard.

Concept references: `docs/visualizer/concepts/*.png`.
