**To:** benjaminbethurum@gmail.com  
**Subject:** MURMUR v1.0.9 — consolidated Mod Matrix + dual concentric knobs

---

Hi Ben,

Quick drop from **Curtis** — **MURMUR v1.0.9** is on GitHub with a bunch of performance UI you asked for (routing you can read at a glance, fewer panel hops, MP11SE-friendly controls).

**Download:** [MURMUR v1.0.9 release](https://github.com/cbackstrom80/murmur-app/releases/tag/v1.0.9)  
Grab **`MURMUR-1.0.9-macOS-arm64.pkg`** (or the `.dmg` if you prefer — same installer inside).

---

### What's new in 1.0.9 (vs earlier builds I sent)

| Highlight | What to try |
|-----------|-------------|
| **Consolidated MOD MATRIX** | PLAY **MOD** tab *or* DESIGN **Matrix** — one reference-style table + modulators, deluxe row styling |
| **Dual concentric knobs** | One dial, two params — **Filter Cutoff/Res**, **WT Bend/Asym** (outer ring + inner hub) |
| **Compact ◎ teleprompter** | Header **◎** — scope hub + KOINS orbit + preset mission card; less chrome for live playing |
| **Live Topology strip** | Tap the mini graph under KOINS → fullscreen algorithm view; edges pulse with MW/EXP |
| **100 Interstellar presets** | BROWSE → **TYPE → Interstellar** (~100 cinematic patches, 900 factory total) |
| **Interstellar HUD badges** | Coordinate tick + **INTERSTELLAR** capsule in preset bar and browser rows |

Still in the box from prior releases: full warp suite (Bend/Asym/Sync/Formant), DESIGN Graph/FX/Wavetable panels, MP11SE CC map, 1.0.6.1 expression + graph-apply fixes.

---

### Install (5 minutes)

1. Download **`MURMUR-1.0.9-macOS-arm64.pkg`**
2. Double-click and follow the installer (home folder, **no admin password**)
3. **Quit Logic completely**, reopen
4. **Logic → Settings → Plug-in Manager → Reset & Rescan Selection**
5. Software Instrument track → **AU Instruments → Murmur → MURMUR**
6. Hit **BROWSE** or load from the preset bar

Gatekeeper block? **Right-click `.pkg` → Open → Open**

Installed docs:

```
~/Library/Application Support/MURMUR/Docs/
~/Library/Application Support/MURMUR/Docs/product/
```

---

### Must-try (15 minutes)

1. **MOD tab** — open PLAY **MOD**, scan a few routes, tweak depth on something you can hear (filter or warp)
2. **Filter concentric dial** — FILTER tab, one knob: outer = cutoff, inner = resonance; sweep while holding a pad
3. **CATHEDRAL NEBULA** — hold C3–C4, mod wheel bloom, optional **◎** compact mode for a cleaner stage view
4. **Compact ◎ mode** — header compact icon; confirm KOINS + scope still readable from the MP11SE bench

---

### MP11SE reminder

| Control | CC | Role |
|---------|-----|------|
| Mod wheel | CC1 | Filter + patch mod routes |
| Expression | CC11 | Resonance / Macro 2 |
| Knob A–D | CC74/71/73/91 | Macros 3–6 |
| Sustain | CC64 | Hold |

Full zone setup: **`KAWAI_MP11SE.md`** in installed docs (knobs → brightness/filter via mod matrix).

---

### Feedback

However is easiest:

- **Reply to this email** — rough notes welcome
- **GitHub Issues:** https://github.com/cbackstrom80/murmur-app/issues

Especially curious: Mod Matrix readability live, concentric filter feel from the wheel + knobs, and whether compact mode works for your rig.

Thanks for testing, Ben — hope this one feels closer to “performance first.”

Cheers,  
**Curtis**
