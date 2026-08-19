# QUASAR 1.0.0 — Standalone binaural spatial effect

First public release of **QUASAR**, extracted from MURMUR's master-bus BinauralSpace slot. Pairs with MURMUR **Interstellar/Spatial** presets for full headphone-first 3D scenes.

## Product

| Property | Value |
|----------|-------|
| Bundle ID | `com.patchwork.quasar` |
| Formats | AU (this release pkg), VST3 + Standalone in repo build |
| AU validation | `auval -v aufx Qsar Murr` |
| Core DSP | `BinauralSpaceProcessor` (QSR1/QSR2, CNTR, room, delay sync) |

## Included

- **QUASAR AU** — insert on master bus after MURMUR
- **76 factory `.quasar` presets** (75 Interstellar Spatial companions + showcase)
- Obsidian UI: spatial knobs, delay tempo sync, output mode, crossfeed
- **AU sidechain input** (MVP: summed with main before processing)

## Logic workflow

1. MURMUR on instrument — load `Interstellar/Spatial/002-void-cathedral.pw8`
2. QUASAR on master bus
3. Load `002-void-cathedral.quasar` (from Application Support or plugin preset folder)
4. Optional: route a bus to QUASAR **Sidechain** for discrete mono → 3D foundation
5. Test on **headphones**

## Install

Download **`QUASAR-1.0.0-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases/tag/quasar-v1.0.0).

Requires **MURMUR v1.2.0+** for migrated Spatial `.pw8` presets (Reverb fallback without QUASAR).

After install: quit Logic → Plug-in Manager → Reset & Rescan → confirm **1.0.0**.

## Build from source

```bash
cmake --preset quasar-release && cmake --build --preset quasar-release
scripts/install_quasar_au_local.sh
```

## Docs

- [`docs/QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md)
- [`docs/GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md)
