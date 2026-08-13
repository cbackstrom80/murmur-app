# MURMUR 1.0.6.1 — Expression mod & graph apply bugfix

Patch release correcting two regressions shipped in **1.0.6** (plugin binary was built without these fixes in the initial 1.0.6 pkg).

## Fixes

### Expression mod source clamp (Interstellar CC11 routes)

- Clamps expression modulation source routing so CC11 / expression-driven mod paths behave correctly on Interstellar and other presets that rely on expression as a mod source.

### Algorithm graph apply — APVTS sync before load

- `commitAlgorithmGraph` now calls `syncCurrentPatchFromApvts` **before** `loadPatch`, so applying a graph from the UI does not revert APVTS-backed parameter state.

## Tests

- Unit/serialization coverage for expression mod source clamping
- Algorithm graph commit tests for APVTS sync ordering

## Install

Download **`MURMUR-1.0.6.1-macOS-arm64.pkg`** (or `.dmg`) from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases). Double-click the installer, quit Logic, rescan AU if prompted.

## Unchanged from 1.0.6

- Interstellar preset install / postinstall rsync fix
- 900 factory presets (800 core + 100 Interstellar), Warp/Templates demos
- All 1.0.5 feature content
