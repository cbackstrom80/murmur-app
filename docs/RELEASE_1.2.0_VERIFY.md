# MURMUR v1.2.0 + QUASAR v1.0.0 — Release Verification

**Branch:** `cursor/favorites-unison-stack-daw`  
**Tags:** `v1.2.0`, `quasar-v1.0.0`  
**Date:** 2026-08-14  

## Version

| Check | MURMUR | QUASAR |
|-------|--------|--------|
| `CMakeLists.txt` / plugin VERSION | **1.2.0** | **1.0.0** (`quasar_plugin/CMakeLists.txt`) |
| Built AU `Info.plist` | **1.2.0** | **1.0.0** |
| `auval` | PASS (`aumu Murm Murr`) | PASS (`aufx Qsar Murr`) |

## Build & test

| Step | Result |
|------|--------|
| `cmake --preset plugin-release` + MURMUR AU build | **PASS** |
| `cmake --preset quasar-release` + QUASAR AU build | **PASS** |
| `ctest --preset dev` | **PASS** — 246/246 |
| `./scripts/build_release_pkg.sh --dmg 1.2.0` | **PASS** |
| `./scripts/build_release_quasar_pkg.sh --dmg 1.0.0` | **PASS** |
| `./scripts/install_au_local.sh` | **PASS** |
| `./scripts/install_quasar_au_local.sh` | **PASS** (after `aufx` fix) |

## Subsystems (1.2.0)

| Area | Result |
|------|--------|
| Quasar removed from MURMUR FX picker | **PASS** |
| Legacy type 11 → Reverb on load | **PASS** |
| 75 Spatial `.pw8` companion metadata | **PASS** |
| 76 `.quasar` factory presets | **PASS** |
| Vocoder / delay sync / morph (1.1.x) | **PASS** — carried forward |

## GitHub releases

| Release | Assets |
|---------|--------|
| [v1.2.0](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.2.0) | `.pkg`, `.dmg`, `version.json` |
| [quasar-v1.0.0](https://github.com/cbackstrom80/patchwork-eight/releases/tag/quasar-v1.0.0) | `.pkg`, `.dmg`, `quasar-version.json` |

## Post-release (v1.2.1 backlog)

| Item | Status |
|------|--------|
| Compressor VCA/FET/Opto in APVTS + PLAY UI | Engine **done**; APVTS/UI **pending** |
| GR meter tap | **Pending** |
| Reverb character bundles (Plate/Hall/Room/Spring) | **P1 — v1.3.0** |
| Default master chain preset migration (Rev→EQ→Comp→Lim) | **Pending** |

**Overall: RELEASED**
