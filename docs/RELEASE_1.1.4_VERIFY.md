# MURMUR v1.1.4 — Release Verification

**Branch:** `cursor/favorites-unison-stack-daw`  
**Tag:** `v1.1.4`  
**Date:** 2026-08-14  

## Version

| Check | Result |
|-------|--------|
| `CMakeLists.txt` `project(VERSION)` | **1.1.4** |
| Built AU `Info.plist` (plugin-release) | **1.1.4** / **1.1.4** |
| `dist/MURMUR-1.1.4-macOS-arm64.pkg` payload AU | **1.1.4** |

## Build & test

| Step | Result |
|------|--------|
| `cmake --preset plugin-release` + build | **PASS** |
| `ctest --preset dev` | **PASS** — 248/248 run, 1 skipped (golden regenerate) |
| `./scripts/build_release_pkg.sh --dmg 1.1.4` | **PASS** (pkg); DMG built manually after transient `hdiutil` busy |
| `./scripts/install_au_local.sh` | See post-release step |

## Subsystems (1.1.1–1.1.4)

| Area | Result |
|------|--------|
| GLOBAL panel | **PASS** — CHAIN / QUASAR / OUTPUT |
| Morph all 75 Spatial | **PASS** |
| Delay tempo sync | **PASS** — Tape + Quasar |
| Mod ghost + ring hierarchy | **PASS** — [`KNOB_RING_SEMANTICS.md`](KNOB_RING_SEMANTICS.md) |
| Vocoder FX type 12 | **PASS** — sidechain modulator, EffectsTests |
| iPad research | **DOC ONLY** — [`IPAD_PORT_RESEARCH.md`](IPAD_PORT_RESEARCH.md) |

## Artifacts

- `dist/MURMUR-1.1.4-macOS-arm64.pkg`
- `dist/MURMUR-1.1.4-macOS-arm64.dmg`
- `dist/version.json`

**Overall: RELEASE READY**
