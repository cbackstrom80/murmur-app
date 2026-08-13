# Week 7 — Logic Pro DAW soak checklist (Ben sign-off prep)

**Branch:** `cursor/favorites-unison-stack-daw`  
**Scope:** DESIGN Wavetable tab + warp panel, PLAY ↔ DESIGN mode switch, preset warp round-trip  
**Host:** Logic Pro on Apple Silicon Mac  
**Keyboard:** Kawai MP11SE (optional — same CC map as [KAWAI_MP11SE.md](KAWAI_MP11SE.md))

Install the Week 7 build:

```bash
cmake --preset plugin-release && cmake --build --preset plugin-release
# Install AU to user Components (or run the release pkg when tagged)
cp -R build/plugin/plugin/pw8_plugin_artefacts/Release/AU/MURMUR.component \
  ~/Library/Audio/Plug-Ins/Components/
```

Quit Logic completely, reopen, **Plug-in Manager → Rescan**.

---

## 1. PLAY ↔ DESIGN mode switch (no glitches)

| # | Step | Pass? |
|---|------|-------|
| 1.1 | Load factory preset `Warp/warp-bend-demo.pw8` (or any wavetable preset) | ☐ |
| 1.2 | Hold a mid-range note (C3) | ☐ |
| 1.3 | Toggle **PLAY → DESIGN → PLAY** three times while note held | ☐ |
| 1.4 | **Expect:** no click/pop, no stuck notes, audio continues | ☐ |
| 1.5 | Repeat with 4-note chord (pad preset) | ☐ |

---

## 2. DESIGN Wavetable tab — preview + warp panel

| # | Step | Pass? |
|---|------|-------|
| 2.1 | Switch to **DESIGN**, open **Wavetable** tab | ☐ |
| 2.2 | Select operator **0** (node pill row) — set engine to **Wavetable** if needed (PLAY OSC or graph) | ☐ |
| 2.3 | **Expect:** 3D stack mesh visible; factory wavetable name in caption | ☐ |
| 2.4 | Use **< / >** arrows to cycle wavetables — mesh updates | ☐ |
| 2.5 | Turn **WT Bend** — mesh shape changes (matches PLAY OSC preview) | ☐ |
| 2.6 | Turn **Sync Ratio** (1→4) and **Sync Amt** — audible sidebands on held note | ☐ |
| 2.7 | **Formant** knob — vowel tables (`formant-vowel-aa`) shift timbre | ☐ |
| 2.8 | Select non-Wavetable operator — **Expect:** hint text, warp knobs hidden | ☐ |

---

## 3. Preset save / reload (warp params)

| # | Step | Pass? |
|---|------|-------|
| 3.1 | Op 0 Wavetable: set Bend +0.5, Sync Ratio 3, Sync Amt 0.7, Formant +0.3 | ☐ |
| 3.2 | **SAVE...** patch to `~/Desktop/week7-warp-test.pw8` | ☐ |
| 3.3 | Load **init** or different preset, then **LOAD...** the saved file | ☐ |
| 3.4 | **Expect:** knobs restore; same timbre on C3 | ☐ |
| 3.5 | Save Logic project, close, reopen — **Expect:** host state restores warp values | ☐ |

---

## 4. Mod matrix + DESIGN integration

| # | Step | Pass? |
|---|------|-------|
| 4.1 | DESIGN **Matrix** tab: add LFO1 → Op0 **WT Bend**, amount 0.5 | ☐ |
| 4.2 | **Expect:** audible LFO motion on bend; ring on DESIGN warp knob | ☐ |
| 4.3 | PLAY OSC page: same operator shows matching mod ring on WT Bend | ☐ |

---

## 5. Filter 2 + scope regression (H1 carry-over)

| # | Step | Pass? |
|---|------|-------|
| 5.1 | PLAY **FILTER** tab: enable Filter 2, sweep cutoff on held bass note | ☐ |
| 5.2 | Oscilloscope on FILTER tab shows live trace | ☐ |
| 5.3 | 15-minute playback with transport loop — no dropouts, no memory climb (Activity Monitor) | ☐ |

---

## 6. pluginval (maintainer — optional on Ben machine)

```bash
chmod +x scripts/run_pluginval.sh
./scripts/run_pluginval.sh
```

**Expect:** `PASS: pluginval strictness 5 (VST3 + AU)`

---

## Sign-off

| Role | Name | Date | Notes |
|------|------|------|-------|
| Engineering | | | Week 7 automated tests green |
| Ben (Logic) | | | Sections 1–5 complete |

**Deferred to Week 8+:** v2 mirror/fold warps, embedded wavetable builder, sampler engine.
