**To:** benjaminbethurum@gmail.com  
**From:** Curtis  
**Subject:** MURMUR v1.2.0 + QUASAR v1.0.0 — two-plugin Spatial workflow

---

Hi Ben,

Two releases today — a **major MURMUR update** and a **new standalone QUASAR plugin** for binaural spatial.

**Downloads:**
- [MURMUR v1.2.0](https://github.com/cbackstrom80/murmur-app/releases/tag/v1.2.0) — **`MURMUR-1.2.0-macOS-arm64.pkg`**
- [QUASAR v1.0.0](https://github.com/cbackstrom80/murmur-app/releases/tag/quasar-v1.0.0) — **`QUASAR-1.0.0-macOS-arm64.pkg`**

Install **both** for the full Interstellar Spatial experience.

---

### What changed in MURMUR 1.2.0

**QUASAR is no longer inside MURMUR.** The binaural spatial engine moved to its own AU effect plugin. MURMUR keeps everything else from 1.1.4 (vocoder, delay sync, mod rings, morph, sidechain mod, 1,079 presets).

| Change | Detail |
|--------|--------|
| **QUASAR removed** | No QUASAR in FX list or GLOBAL tab |
| **Spatial presets migrated** | 75 Interstellar/Spatial pads now use **Reverb** on M3; metadata points to a companion `.quasar` file |
| **Two-plugin workflow** | MURMUR = synth + KOINS; QUASAR = master-bus spatial on headphones |

---

### QUASAR 1.0.0 (new plugin)

- **AU Effects → Murmur → QUASAR** on master bus **after** MURMUR
- Full binaural DSP: QSR1/QSR2 paths, CNTR anchor, room, tempo-synced delay
- **76 `.quasar` presets** — 75 match Interstellar Spatial pads by name
- **Sidechain input** on AU (route a bus for future mono → 3D work)

---

### Try it (~15 minutes)

1. Install both `.pkg` files (user folder — no admin password)
2. **Quit Logic completely**, reopen, **Reset & Rescan** both plugins
3. Confirm versions: MURMUR **1.2.0**, QUASAR **1.0.0**
4. Instrument track → **MURMUR** → load **`Interstellar/Spatial/002-void-cathedral`**
5. Master bus → **QUASAR** → load **`002-void-cathedral.quasar`**
6. **Headphones** — sweep Morph on MURMUR, Distance/Angle on QUASAR
7. Optional: route vocal bus to QUASAR **Sidechain**

Without QUASAR, Spatial pads still play — M3 is algorithmic Reverb, not silent.

---

### Still in MURMUR from 1.1.x

- Vocoder FX (sidechain-driven)
- Tape delay tempo sync
- Mod ghost pointers + knob ring legend
- Morph on all 75 Spatial pads
- Sidechain → mod matrix

---

### Feedback

Reply here or GitHub Issues — especially:
- Does the two-plugin setup feel natural in your Logic template?
- Spatial scenes with vs without QUASAR on headphones
- Whether Reverb-only fallback is acceptable for live sets without QUASAR loaded

Cheers,  
**Curtis**

---

*Preset path after QUASAR install:* `~/Library/Application Support/QUASAR/Presets/quasar/interstellar/`
