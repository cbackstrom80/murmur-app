# MURMUR 1.2.0 — QUASAR extracted, Spatial preset migration

Major release on branch `cursor/favorites-unison-stack-daw`. **Binaural spatial (QUASAR) is no longer inside MURMUR** — it ships as the standalone [**QUASAR v1.0.0**](https://github.com/cbackstrom80/patchwork-eight/releases/tag/quasar-v1.0.0) effect plugin.

## What's new in 1.2.0

### QUASAR removed from MURMUR

- **`EffectType::BinauralSpace` removed** — legacy type 11 loads as **Reverb** on old patches
- **GLOBAL QUASAR tab removed** — CHAIN / OUTPUT remain in Advanced GLOBAL
- **Quasar APVTS params and mod destinations removed** from MURMUR
- MURMUR keeps: synth, KOINS, Reverb (M7 FDN), Tape, Vocoder, Chorus, EQ, Compressor, Limiter

### Interstellar Spatial preset migration

- **75 Spatial `.pw8` presets** updated with metadata `"spatial": "use-quasar-plugin"`
- Each preset references a **companion `.quasar` file** (`companionQuasar` in metadata)
- Master M3 fallback: algorithmic **Reverb** (SPACE macro KOINS still work in MURMUR-only)
- Companion scenes: `content/presets/quasar/interstellar/*.quasar` (bundled with QUASAR installer)

### Two-plugin Logic workflow

1. **MURMUR** — instrument track (KOINS, synth, M7 reverb fallback)
2. **QUASAR** — master bus after MURMUR (headphones-first binaural spatial)
3. Load matching `.quasar` for full Spatial scene (e.g. `002-void-cathedral.quasar`)

## Carried from 1.1.4

- **8-band sidechain Vocoder** FX
- **Delay tempo sync** (Tape Delay)
- **Mod ghost pointers** + knob ring hierarchy
- **Morph KOIN** on all 75 Spatial pads
- **Sidechain → mod matrix** (AU)
- **1,079** factory presets

## Install

Download **`MURMUR-1.2.0-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.2.0).

For full Spatial scenes, also install [**QUASAR v1.0.0**](https://github.com/cbackstrom80/patchwork-eight/releases/tag/quasar-v1.0.0).

After install: quit Logic → Plug-in Manager → Reset & Rescan → confirm **1.2.0**.

## Docs

| Topic | Doc |
|-------|-----|
| Timeline | [`docs/MURMUR_ACCOMPLISHMENTS.md`](MURMUR_ACCOMPLISHMENTS.md) |
| QUASAR standalone | [`docs/QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md) |
| Post-Quasar FX plan | [`docs/SYNTH_BUNDLED_FX_RESEARCH.md`](SYNTH_BUNDLED_FX_RESEARCH.md) |
| Vocoder | [`docs/VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md) |
