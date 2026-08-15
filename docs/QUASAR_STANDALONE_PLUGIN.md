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
Main Input (stereo) ──┐
                      ├──► [optional sum] ──► BinauralSpaceProcessor ──► Output (stereo)
Sidechain (AU, stereo) ┘
```

### Sidechain (MVP)

On AU builds, a second stereo input bus **Sidechain** is exposed. For MVP, when sidechain audio is present it is **summed with the main input** before processing. This establishes the foundation for future **QSR2 aux** routing (sidechain-only into QSR2 path). VST3 sidechain support is deferred.

### Tempo sync

Delay time sync reads BPM from the host playhead (`AudioPlayHead::getPosition()`). When `quasarDelaySync` is on, effective delay ms is derived from the selected division and host tempo.

---

## Parameters (25)

All parameters are flat APVTS floats (discrete params use integer steps). IDs match MURMUR master FX Quasar field names.

| Group | Parameters |
|-------|------------|
| **Mix** | `mix` |
| **Balance** | `qsr1Level`, `qsr2Level`, `cntrLevel` |
| **Input split** | `inputSplitHpfHz`, `cntrHpfHz` |
| **QSR1 spatial** | `qsr1Height`, `qsr1Angle`, `qsr1Distance` |
| **QSR2 spatial** | `qsr2Height`, `qsr2Angle`, `qsr2Distance` |
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

- Factory presets: `content/presets/quasar/*.quasar`
- Host state save/load uses the same JSON envelope (params only, no metadata required)
- Example: `001-orbit-cathedral.quasar` — migrated from Interstellar **VOID CATHEDRAL** spatial character

---

## UI (MVP)

Obsidian skin adapted from MURMUR `GlobalPanel` QUASAR tab:

- **ObsidianLookAndFeel**, **GlowKnob**, **MetadataFacetRow** (minimal copy under `quasar_plugin/src/ui/`)
- Spatial knobs: Distance, Angle, Height, Room, CNTR/QSR levels
- Delay sync rows (FREE/TEMPO + division chips)
- Output mode + crossfeed
- Spherical scope placeholder (Phase 4)

Editor size: 920×640.

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

1. **QSR2 aux** — route sidechain to QSR2 only (not summed with main)
2. **Spherical scope** — live QSR1/QSR2 position visualization
3. **Preset browser** — load/save `.quasar` from UI
4. **VST3 sidechain** — host-dependent sidechain bus
5. **Mod matrix / LFO** — internal orbit motion (Phase 3 destinations from GLOBAL plan)
6. **Quality tiers** — Eco/Normal/High HRTF (see GLOBAL plan §2.7)

---

## Related docs

- [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) — full Quasar DSP + MURMUR integration plan
- [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md) — research reference
- [`PLUGIN_ARCHITECTURE.md`](PLUGIN_ARCHITECTURE.md) — MURMUR JUCE wrapper patterns
