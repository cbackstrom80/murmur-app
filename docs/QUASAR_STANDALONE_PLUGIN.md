# QUASAR Standalone Plugin — Product Spec (MVP)

**Status:** MVP scaffold — buildable VST3/AU/Standalone effect wrapping `effects::BinauralSpaceProcessor`  
**Build:** `cmake --preset quasar-release && cmake --build --preset quasar-release`  
**Install AU (macOS):** `scripts/install_quasar_au_local.sh`

---

## Product summary

**QUASAR** is a standalone JUCE audio effect plugin extracted from MURMUR's master-bus `BinauralSpace` (QUASAR) slot. It is a **headphone-first binaural spatial mixer** with dual QSR paths, CNTR dry anchor, embedded per-path room, and post-sum stereo delay — see [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) for full DSP architecture.

| Property | Value |
|----------|-------|
| Product name | QUASAR |
| CMake target | `pw8_quasar_plugin` |
| Bundle ID | `com.patchwork.quasar` |
| Plugin code | `Qsar` (AU four-char) |
| Manufacturer | `Murr` |
| Formats | VST3, AU, Standalone |
| Type | Effect (`kAudioUnitType_Effect`) |
| Core DSP | `pw8::effects::BinauralSpaceProcessor` |

---

## Signal flow

```
Main Input (stereo L/R)
    │
    ├── L (HPF) ──► QSR1 path (binaural pan + room) ──┐
    ├── R (HPF) ──► QSR2 path (binaural pan + room) ──┼──► sum + delay ──► Output
    └── CNTR (HPF L/R anchor) ──────────────────────────┘

Sidechain (AU, stereo) — **QSR2-only aux** (default) or legacy sum-with-main (`sidechainToQsr2` off)
```

**Stereo split (always on in standalone QUASAR):** the left input channel feeds QSR1 only; the right channel feeds QSR2 only. This lets you place each side of a stereo field independently in headphone space. MURMUR's legacy master-bus path still uses summed mono to both QSR paths when `qsrStereoSplit` is false.

### Sidechain (MVP)

On AU builds, a second stereo input bus **Sidechain** is exposed. For MVP, when sidechain audio is present it is **summed with the main input** before processing. This establishes the foundation for future **QSR2 aux** routing (sidechain-only into QSR2 path). VST3 sidechain support is deferred.

### Tempo sync

Delay time sync reads BPM from the host playhead (`AudioPlayHead::getPosition()`). When `quasarDelaySync` is on, effective delay ms is derived from the selected division and host tempo.

---

## Parameters (28)

All parameters are flat APVTS floats (discrete params use integer steps). IDs match MURMUR master FX Quasar field names where applicable.

| Group | Parameters |
|-------|------------|
| **Mix** | `mix` |
| **Balance** | `qsr1Level`, `qsr2Level`, `cntrLevel` |
| **Input split** | `inputSplitHpfHz`, `cntrHpfHz` |
| **QSR1 spatial** | `qsr1Height`, `qsr1Angle`, `qsr1Distance` |
| **QSR2 spatial** | `qsr2Height`, `qsr2Angle`, `qsr2Distance` |
| **Macro KOINS** | `orbitMacro`, `spreadMacro` (0.5 = neutral; fan to both QSR paths at DSP) |
| **Sidechain** | `sidechainToQsr2` (on = AU sidechain feeds QSR2 only) |
| **QSR1 room** | `qsr1RoomAmount`, `qsr1RoomSize`, `qsr1RoomDamping` |
| **QSR2 room** | `qsr2RoomAmount`, `qsr2RoomSize`, `qsr2RoomDamping` |
| **Delay** | `quasarDelayTimeMs`, `quasarDelayFeedback`, `quasarDelayVolume`, `quasarDelaySync`, `quasarDelaySyncDivision` |
| **Output** | `quasarOutputMode` (Headphone/Speaker/Auto), `quasarCrossfeed` |

Defaults match `effects::EffectSlotParams` Quasar defaults in `engine/include/pw8/effects/EffectTypes.hpp`.

---

## Preset format (`.quasar`)

JSON, `schemaVersion: 1`:

```json
{
  "schemaVersion": 1,
  "metadata": { "name": "...", "author": "...", "description": "..." },
  "params": { "mix": 0.72, "qsr1Level": 0.65, ... }
}
```

- Factory presets: `content/presets/quasar/*.quasar` and subfolders:
  - `interstellar/` — 75 MURMUR Spatial companions
  - `play/` — 20 PLAY-surface showcases (stereo split, macros, sidechain)
- Host state save/load uses the same JSON envelope (params only, no metadata required)
- Regenerate PLAY batch: `python3 scripts/generate_quasar_play_presets.py`
- Example: `001-orbit-cathedral.quasar` — migrated from Interstellar **VOID CATHEDRAL** spatial character

---

## UI (MVP)

Obsidian skin adapted from MURMUR `GlobalPanel` QUASAR tab:

- **PLAY surface**
  - **Macro KOINS:** ORBIT (`orbitMacro`) rotates both feeds ±180°; SPREAD (`spreadMacro`) widens/narrows angle + depth + height
  - **6 spatial knobs:** L/R ORBIT · DEPTH · LIFT (direct APVTS)
- **Wireframe scope** — draggable L/R markers write spatial params; glow tethers from listener head
- **SC AUX row** — QSR2 (sidechain-only into QSR2 path) vs SUM (legacy add-to-main)
- **DEEP panel** — mix, CNTR, room, delay, output mode, HPFs

Editor size: 920×720.

---

## Build system

| Option | Default | Description |
|--------|---------|-------------|
| `PW8_BUILD_QUASAR_PLUGIN` | OFF | Enable `quasar_plugin/` subdirectory |

CMake preset **`quasar-release`**: Release arm64, `PW8_BUILD_QUASAR_PLUGIN=ON`, `PW8_BUILD_PLUGIN=OFF`.

Links: `pw8::core`, JUCE (`juce_audio_utils`, `juce_dsp`). Branding icon reused from `plugin/resources/branding/murmur_mark_512.png`.

---

## Validation

| Check | Command |
|-------|---------|
| Configure + build | `cmake --preset quasar-release && cmake --build --preset quasar-release` |
| AU install + auval | `scripts/install_quasar_au_local.sh` |
| auval subtype | `auval -v aufx Qsar Murr` |

---

## Roadmap (post-MVP)

1. ~~**QSR2 aux** — route sidechain to QSR2 only~~ (shipped)
2. ~~**Draggable scope**~~ (shipped)
3. ~~**Macro KOINS** — ORBIT/SPREAD~~ (shipped)
4. **Preset browser** — load/save `.quasar` from UI
5. **VST3 sidechain** — host-dependent sidechain bus
6. **Mod matrix / LFO** — internal orbit motion
7. **Quality tiers** — Eco/Normal/High HRTF (see GLOBAL plan §2.7)

---

## Related docs

- [`QUASAR_FIGMA_BUILD_GUIDE.md`](QUASAR_FIGMA_BUILD_GUIDE.md) — Figma build order, region map, param→control table, preset fixtures
- [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) — full Quasar DSP + MURMUR integration plan
- [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md) — research reference
- [`PLUGIN_ARCHITECTURE.md`](PLUGIN_ARCHITECTURE.md) — MURMUR JUCE wrapper patterns
