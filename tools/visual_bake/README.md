# FX Visual Atlas Bake Tool

Offline helper for generating horizontal sprite atlases used by MURMUR CPU FX previews.

The plugin also **bakes atlases at runtime** on first use (`FxAnimationAtlas`), so this tool is optional — use it when you want committed PNG assets or to iterate on look outside the plugin.

## Quick start

```bash
python3 tools/visual_bake/bake_fx_atlases.py --out plugin/resources/fx_atlases
```

Outputs:

- `chorus_atlas.png` — 24 frames, L/R chorus motion
- `tape_atlas.png` — wow/flutter drift line
- `reverb_atlas.png` — decay envelope with shimmer
- `clouds_atlas.png` — granular particle field
- `manifest.json` — frame count, dimensions, param keys

## Integration

Runtime atlases are baked on first use; committed PNGs load from BinaryData when param keys match defaults (see `FxEmbeddedAtlases`).

Default embedded params: chorus 1 Hz / 5 ms / 0.5 mix; tape 0.5 / 5 / 0.5; reverb 2 s / 0.5 size / 0.5 damp / 0.5 mix; clouds 0.5 mix.

## Params hashed into cache keys

| Effect | Key inputs |
|--------|------------|
| Chorus | rate, depth, mix |
| Tape | drift rate, depth, mix |
| Reverb | decay, size, damping, mix |
| Clouds | mix, seed |

Quantization matches `VisualPreviewCache::quantizeToKey()` (32 steps).
