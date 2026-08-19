# DAW Host Matrix

Manual validation checklist for MURMUR as a VST3/AU instrument across
common macOS hosts. Run after plugin packaging changes, JUCE upgrades, or
preset/content path work.

**Build under test:** local `main` + `cursor/favorites-unison-stack-daw` (2026-08-12)  
**Plugin build:** VST3 / AU / Standalone  
**Tester:** automated + pending manual DAW pass  
**Date:** 2026-08-12

## Automated validation (2026-08-12)

| Check | Result |
|-------|--------|
| Unit/regression tests (`ctest --preset dev`) | **169/169 PASS** |
| pluginval VST3 strictness 5 | **PASS** |
| auval AU (`aumu` / `Pwe8` / `Pwei`) | **PASS** |
| Logic / Ableton / Reaper / Bitwig manual matrix | **Pending** (requires human host session) |

Notes:
- VST3/AU binaries built from `build/plugin` (`pw8_plugin_VST3`, `pw8_plugin_AU`).
- Standalone link may fail if `build/plugin/.../Standalone/` is root-owned from a prior `sudo` install — run `sudo chown -R "$(whoami)" build/plugin` if needed.
- Manual DAW rows below remain unchecked until tested in each host.

## Install paths (macOS)

| Artifact | Expected location |
|----------|-------------------|
| VST3 | `/Library/Audio/Plug-Ins/VST3/MURMUR.vst3` |
| AU | `/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| Wavetables | `/Library/Application Support/MURMUR/Wavetables/` |
| Factory presets | `/Library/Application Support/MURMUR/Presets/` |

Rescan plugins in each DAW after install. Confirm `PW8_CONTENT_ROOT` is **not**
set unless deliberately testing dev-tree content.

---

## Smoke test (all hosts)

Run once per host before deep checks.

- [ ] Plugin appears in instrument list (VST3 and/or AU as applicable)
- [ ] Insert on instrument track; UI opens without crash
- [ ] Default INIT patch produces audio on MIDI input
- [ ] Window size ~980×920; tabs BASIC / OSC / FILTER / ENV / MOD / FX switch
- [ ] Prev / Next / BROWSE preset navigation loads factory presets
- [ ] Save/restore session: close project, reopen — plugin state persists
- [ ] Bypass works; un-bypass restores audio

---

## Host matrix

| Check | Logic Pro | Ableton Live | Reaper | Bitwig |
|-------|-----------|--------------|--------|--------|
| Scan / insert | ☐ | ☐ | ☐ | ☐ |
| AU loads (if tested) | ☐ | ☐ | ☐ | ☐ |
| VST3 loads | ☐ | ☐ | ☐ | ☐ |
| MIDI input / note on | ☐ | ☐ | ☐ | ☐ |
| Preset browse overlay | ☐ | ☐ | ☐ | ☐ |
| Prev/next respects filter | ☐ | ☐ | ☐ | ☐ |
| Wavetable ops audible (WT, Granular) | ☐ | ☐ | ☐ | ☐ |
| FX chain audible | ☐ | ☐ | ☐ | ☐ |
| Automation (macro or cutoff) | ☐ | ☐ | ☐ | ☐ |
| Multi-instance (2+ on separate tracks) | ☐ | ☐ | ☐ | ☐ |
| Offline bounce / export | ☐ | ☐ | ☐ | ☐ |
| CPU reasonable at 512 buffer | ☐ | ☐ | ☐ | ☐ |

**Notes per host:**

### Logic Pro
- Version: _______________
- Issues:

### Ableton Live
- Version: _______________
- Issues:

### Reaper
- Version: _______________
- Issues:

### Bitwig
- Version: _______________
- Issues:

---

## Regression triggers

Re-run this matrix when any of the following change:

- JUCE version or plugin format flags
- `ContentPaths` / installer preset or wavetable layout
- APVTS parameter layout or preset serialization
- PLAY UI (preset browser, tab layout, envelope panel)
- Audio graph or engine load path

---

## Known limitations (expected, not bugs)

Document here if observed during testing:

- Layer modes other than **SingleA** and **STACK** are still clamped at load (Split, Morph, etc.)
- Unison supports **Full** spread mode only (`voices` + detune/spread/blend); Operator/Stereo/Hyper/Harmonic modes deferred
- MCP in-app chat deferred (external MCP server only)
- Favorites persist to `~/Library/Application Support/MURMUR/favorites.json`
