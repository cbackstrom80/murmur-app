# Week 8 — Program exit checklist (Curtis + Ben sign-off)

**Branch:** `cursor/favorites-unison-stack-daw`  
**Release context:** v1.0.6.1 shipped; Week 8 closes DESIGN + Warps accelerated track exit gates  
**Host:** Logic Pro on Apple Silicon Mac (Ben); maintainer runs pluginval + ctest (Curtis)

Install the Week 8 build:

```bash
cmake --preset plugin-release && cmake --build --preset plugin-release
cp -R build/plugin/plugin/pw8_plugin_artefacts/Release/AU/MURMUR.component \
  ~/Library/Audio/Plug-Ins/Components/
```

Quit Logic completely, reopen, **Plug-in Manager → Rescan**.

---

## 1. Program exit gates (DESIGN_AND_WARPS_PLAN.md §4)

| # | Gate | Engineering | Ben (Logic) |
|---|------|-------------|-------------|
| 1.1 | **Sound design without JSON:** FM + warp motion patch authored entirely in-plugin, saved, reloaded | ☐ Automated: graph commit + serializer tests | ☐ Manual: DESIGN graph edit → save `.pw8` → reload in Logic |
| 1.2 | **Competitive story:** ≥5 demo presets (warp + filter + mod) sound intentional | ☐ Factory Warp + Interstellar packs load | ☐ Audition `Warp/warp-bend-demo`, `Interstellar/001-cathedral-nebula`, `058-wormhole-rise` |
| 1.3 | **Safety:** Invalid graph never reaches audio; fuzz-render clean with warps | ☐ `AlgorithmGraphCommitTests`, `WavetableWarpTests`, fuzz | ☐ N/A (maintainer) |
| 1.4 | **Performance:** Mode switch + warp knobs — no clicks on held notes | ☐ pluginval strictness 5 PASS | ☐ [WEEK7_DAW_SOAK_CHECKLIST.md](WEEK7_DAW_SOAK_CHECKLIST.md) §1 + §2 (regression) |

---

## 2. Week 8 engineering deliverables

| # | Item | Pass? |
|---|------|-------|
| 2.1 | `scripts/run_pluginval.sh` — strictness 5 VST3 + AU PASS | ☐ |
| 2.2 | Sync Ratio drag-to-mod on DESIGN `WavetableWarpPanel` | ☐ |
| 2.3 | Sync Amt drag-to-mod on PLAY `OperatorEditorPanel` | ☐ |
| 2.4 | Golden Interstellar hashes in `tests/golden/presets.json` (5 presets) | ☐ |
| 2.5 | `ctest --preset dev` all green | ☐ |
| 2.6 | PatchSerializerTests comment reflects schema v3 | ☐ |

---

## 3. Sync mod targets (Ben quick check)

| # | Step | Pass? |
|---|------|-------|
| 3.1 | PLAY OSC, Wavetable op 0 — drag LFO1 onto **Sync Amt** knob | ☐ |
| 3.2 | **Expect:** mod ring on Sync Amt; audible sync blend motion | ☐ |
| 3.3 | DESIGN Wavetable tab — drag LFO2 onto **Sync Ratio** knob | ☐ |
| 3.4 | **Expect:** mod ring; sideband motion on held note | ☐ |
| 3.5 | DESIGN Matrix tab lists **WT Sync Amt** destination | ☐ |

---

## 4. Interstellar golden regression (maintainer)

```bash
ctest --preset dev -R GoldenPreset
```

**Expect:** Cathedral Nebula, Cornfield Chase, Wormhole Rise, Pillars of Creation, No Time for Caution hashes match manifest.

---

## 5. pluginval (maintainer)

```bash
chmod +x scripts/run_pluginval.sh
./scripts/run_pluginval.sh
```

**Expect:** `PASS: pluginval strictness 5 (VST3 + AU)`

---

## Sign-off

| Role | Name | Date | Notes |
|------|------|------|-------|
| Engineering (Curtis) | | | pluginval + ctest + golden manifest |
| Product / Logic (Ben) | | | §1 gates + §3 sync mod UX |

**Deferred (Horizon 2+):** v2 mirror/fold warps, embedded wavetable builder, sampler engine, mode-switch accessibility audit.

**Release note:** No v1.0.6.2 cut unless pluginval finds a critical blocker — document in sign-off Notes if needed.
