# MURMUR v1.1.0 — Release Verification

**Branch:** `cursor/favorites-unison-stack-daw`  
**Tag:** `v1.1.0` (HEAD = tag, no post-tag commits)  
**Date:** 2026-08-14  
**Verifier:** automated code check + build/test/install pass

## Subsystem audit

| Subsystem | Result | Notes |
|-----------|--------|-------|
| **1. Quasar Phase 3** | **PASS** | `BinauralSpaceProcessor` in `EffectChain` (`EffectType::BinauralSpace`). Mod destinations enum 21–30 (`QuasarQsr1Distance` … `QuasarCntrLevel`) wired in `ModMatrixExecutor::applyMasterBus` and applied in `Engine.cpp`. PLAY Advanced FX binds APVTS via `FxChainStrip::refreshQuasarUi` (Output Mode, Crossfeed, CNTR, Q2 Dist, Room, Delay Fdbk). DSP: headphone/speaker ITD scale + crossfeed, delay freeze at feedback ≥ 0.99. |
| **2. Morph KOIN** | **PASS** | `applyMorphKoin` on patch load + `morphPosition` APVTS change (`PatchworkEightProcessor::applyMorphFromPosition`). `PatchFocusPanel` + `ModRoutingUi` show morph knob when `uiFocus kind:morph` and ≥2 keyframes. **20** Spatial presets contain `morphKoin` (001–020). **75** total Interstellar Spatial `.pw8` files. |
| **3. Sidechain MVP** | **PASS** | AU-only: `#if JucePlugin_Build_AU` sidechain bus + `SidechainFollower` → `engine->setSidechainLevel`. `ModSource::Sidechain` in executor. PLAY badge via `formatSidechainStatus` in `PatchFocusPanel`. VST3/Standalone compile path zeros sidechain. |
| **4. Feature KOINS** | **PASS** | 1–3 feature macros via `kMinFeatureKoinCount`/`kMaxFeatureKoinCount`; `inferPatchFocusLayout` + route validation (`macroHasActiveRoute`, `ensureMinimumFeatureKnobs`). Dissemination via `macroDissemination` + `disseminationDepth` in engine + `ensureMinimumMacroKoinRoutes`. Standard param knobs with APVTS validation. |
| **5. Master mod matrix** | **PASS** | `ModScope::Global` routes hit `applyMasterBus` (master FX mix/reverb + Quasar offsets). Voice-scoped master routes ignored per tests. |
| **6. Install / version** | **PASS** | `CMakeLists.txt` `VERSION 1.1.0`. Built AU plist: `CFBundleShortVersionString` / `CFBundleVersion` = **1.1.0**. `scripts/install_au_local.sh` → rsync + cache clear + **auval PASS**. |

## Build & test commands

| Step | Result |
|------|--------|
| `ctest --preset dev` | **PASS** — 243/243 run, 1 skipped (golden regenerate) |
| `cmake --build --preset plugin-release` | **PASS** — clean build |
| `./scripts/install_au_local.sh` | **PASS** |
| `auval -v aumu Murm Murr` | **PASS** — AU VALIDATION SUCCEEDED |

## Stub / TODO grep (Quasar / Morph / Sidechain paths)

No `TODO`, `FIXME`, or `stub` markers in:

- `engine/include/pw8/effects/` (BinauralSpace, EffectChain)
- `engine/include/pw8/modulation/` (ModMatrixExecutor, MorphKoinExecutor)
- `plugin/src/processor/PatchworkEightProcessor.*`
- `plugin/src/ui/components/{FxChainStrip,PatchFocusPanel,ModRoutingUi}.*`

## Release artifacts

| Item | Status |
|------|--------|
| GitHub release `v1.1.0` | **Exists** — [releases/tag/v1.1.0](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.1.0) |
| `MURMUR-1.1.0-macOS-arm64.pkg` | Attached |
| `MURMUR-1.1.0-macOS-arm64.dmg` | Attached |
| `version.json` | Not attached (optional; pkg/plist carry version) |

**Version bump decision:** No code changes during verify → **stay at 1.1.0** (no 1.1.1 bump, no re-tag).

## Overall

**RELEASE READY — all subsystems PASS.**
