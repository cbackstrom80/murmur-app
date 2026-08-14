**To:** benjaminbethurum@gmail.com  
**From:** Curtis  
**Subject:** MURMUR v1.1.0 — Quasar spatial morph + sidechain mod

---

Hi Ben,

**MURMUR v1.1.0** is ready — this is the big spatial release. Quasar binaural master FX (built for headphones), live morph between patch keyframes, AU sidechain as a mod source, and a full bank of Interstellar Spatial pads.

**Download:** [MURMUR v1.1.0 release](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.1.0)  
Grab **`MURMUR-1.1.0-macOS-arm64.pkg`** (or the `.dmg` if you prefer drag-and-drop).

---

### What's new in 1.1.0

**PLAY-only UI** — Basic, Compact, and Advanced layouts. The focus panel shows **feature macro KOINS** (1–3 patch-authored macros with custom labels) plus **standard knobs** (filter, master, etc.) when the patch defines them. Dissemination presets spread macro values per voice for lush ensemble pads.

| Highlight | What to try |
|-----------|-------------|
| **Quasar binaural master FX** | Headphones on → Interstellar Spatial preset → Advanced **FX** → master **QUASAR**. Toggle **Output Mode** (Headphone vs Speaker), tweak **Crossfeed**, push delay feedback high for **freeze** |
| **75 Interstellar Spatial pads** | BROWSE → Interstellar → Spatial (001–075). Cathedral-scale room imaging on every one |
| **Dissemination bank** | BROWSE → Dissemination — per-voice macro spread; great for held chords and evolving washes |
| **Morph KOIN (EVOLVE)** | 20 Spatial showcases (001–020) morph INTIMATE ↔ STAGE ↔ VOID live — sweep the **Morph** knob |
| **Sidechain mod (AU only)** | Route a vocal/drum bus into MURMUR's sidechain input; add MOD route **SIDECHAIN →** filter or Quasar; badge pulses when input hits |

Still in the box from 1.0.9: consolidated Mod Matrix, dual concentric knobs, compact teleprompter, 900+ factory presets.

---

### Install (5 minutes)

1. Download **`MURMUR-1.1.0-macOS-arm64.pkg`** from the release page
2. Double-click the installer (installs to your home folder — no admin password)
3. **Quit Logic completely**, reopen
4. **Plug-in Manager → Reset & Rescan Selection** on MURMUR  
   Confirm version shows **1.1.0** in the Plug-in Manager
5. Create a Software Instrument track → **AU Instruments → Murmur → MURMUR**

---

### Sidechain routing in Logic (AU)

1. On the **MURMUR** track: open the plug-in header → set **Sidechain** input to your source track (e.g. vocal or kick)
2. In MURMUR **MOD** tab: add a route — source **SIDECHAIN**, destination e.g. **Filter Cutoff** or **Quasar QSR1 Distance**
3. Play the sidechain source — the **Sidechain (AU)** badge in the PLAY panel should show activity

*(Sidechain input is AU-only; VST3 build ignores the bus.)*

---

### Must-try presets (15 minutes)

1. **001-nebula-drift** (Interstellar/Spatial) — sweep **Morph** (EVOLVE); headphones, listen for room/distance morph  
2. **001-cathedral-nebula** (Interstellar) — massive Quasar room; Advanced → FX → QUASAR  
3. **002-frozen-bloom** (Dissemination) — dissemination spread + held-pad bloom  
4. Toggle **Output Mode** Headphone ↔ Speaker on any Spatial pad; tweak **Crossfeed** in Speaker mode  
5. Hold a pad, push Quasar **Delay Fdbk** past ~99% — tail should **freeze**

---

### MP11SE reminder

| Control | CC | Role |
|---------|-----|------|
| Mod wheel | CC1 | Filter + patch mod routes |
| Expression | CC11 | Resonance / Macro 2 |
| Knob A–D | CC74/71/73/91 | Macros 3–6 |

---

### Feedback

Reply here or open a GitHub Issue — especially morph feel on the Spatial showcases, Quasar imaging on your headphones, and whether the sidechain routing steps are clear in Logic.

Cheers,  
**Curtis**

---

*Curtis sends this manually — copy/paste from this file.*
