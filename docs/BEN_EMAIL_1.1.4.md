**To:** benjaminbethurum@gmail.com  
**From:** Curtis  
**Subject:** MURMUR v1.1.4 — vocoder FX, clearer mod rings, GLOBAL Quasar

---

Hi Ben,

**MURMUR v1.1.4** is ready. This bundles everything since 1.1.0: the **GLOBAL** Quasar panel, **tempo-synced delays**, **mod ghost pointers** on knobs, a clearer **knob ring** hierarchy, and a new **8-band sidechain vocoder** FX you can drop in the chain.

**Download:** [MURMUR v1.1.4 release](https://github.com/cbackstrom80/murmur-app/releases/tag/v1.1.4)  
Grab **`MURMUR-1.1.4-macOS-arm64.pkg`** (or the `.dmg` if you prefer drag-and-drop).

---

### What's new since 1.1.0

| Release | Highlights |
|---------|------------|
| **1.1.1** | Advanced **GLOBAL** tab (CHAIN / QUASAR / OUTPUT); **Morph** on all **75** Interstellar Spatial pads |
| **1.1.2** | **Tape + Quasar delay** tempo sync (1/1 … 1/16, dotted, triplet); **ghost pointer** shows live modulated knob value |
| **1.1.3** | Thicker **value / mod / ghost** rings; BASIC PLAY legend: `● value   ○ mod route   · live mod` |
| **1.1.4** | **VOCODER** master/insert FX — sidechain audio drives 8-band formant shaping on the synth |

Still in the box from 1.1.0: Quasar binaural master FX, Dissemination spread pads, sidechain → mod matrix, 1,079 factory presets.

---

### Try the vocoder (Logic, ~10 minutes)

1. **Sidechain source** — vocal or drum bus (same as sidechain mod from 1.1.0).
2. On **MURMUR**: plug-in header → **Sidechain** = that bus.
3. **Advanced → FX** (or **GLOBAL → CHAIN**): pick an insert or master slot → type **VOCODER**.
4. Play synth + sidechain — tweak **Bands**, **Formant**, **Sibilance**, **SC Gain**, **Mix**.
5. Optional: keep a MOD route **SIDECHAIN → Filter** and watch the **ghost dot** on the cutoff knob (1.1.2+).

If sidechain is silent (standalone), vocoder falls back to self-mod on the carrier for testing.

---

### Install (5 minutes)

1. Download **`MURMUR-1.1.4-macOS-arm64.pkg`** from the release page
2. Double-click the installer (user folder — no admin password)
3. **Quit Logic completely**, reopen
4. **Plug-in Manager → Reset & Rescan Selection** on MURMUR  
   Confirm version **1.1.4**
5. Software Instrument track → **AU Instruments → Murmur → MURMUR**

---

### Must-try presets (15 minutes)

1. Any **Interstellar/Spatial** pad — sweep **Morph** (now on all 75, not just 001–020)
2. **Advanced → GLOBAL → QUASAR** — toggle delay **SYNC**, change **DIV**, compare to FREE ms
3. Assign **LFO → Filter Cutoff** — watch the **ghost pointer** vs the base value arc
4. **Dissemination** held chord + vocoder on master slot with vocal sidechain
5. **Output Mode** Headphone ↔ Speaker on Quasar; delay feedback **freeze** past ~99%

---

### MP11SE reminder

| Control | CC | Role |
|---------|-----|------|
| Mod wheel | CC1 | Filter + patch mod routes |
| Expression | CC11 | Resonance / Macro 2 |
| Knob A–D | CC74/71/73/91 | Macros 3–6 |

---

### Feedback

Reply here or open a GitHub Issue — especially vocoder mix with your vocal chain, whether the knob rings read clearly under stage lighting, and GLOBAL Quasar layout for live tweaks.

Cheers,  
**Curtis**

---

*Curtis sends this manually — copy/paste from this file.*
