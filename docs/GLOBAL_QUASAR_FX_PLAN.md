# Global Quasar FX — Implementation Plan

**Date:** 2026-08-14 (updated 2026-08-15)  
**Branch:** `cursor/favorites-unison-stack-daw`  
**Status:** **PIVOT — standalone QUASAR plugin** (extraction complete)  
**Audience:** Historical MURMUR integration plan + migration notes  

> **Decision (2026-08-15):** Quasar binaural spatial DSP is **no longer a MURMUR master FX slot**. It ships as the standalone **QUASAR** effect plugin (`com.patchwork.quasar`). See [`QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md) for the current product spec, build (`cmake --preset quasar-release`), and AU sidechain routing.

**Related:** [`QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md), [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md), [`MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md), [`HORIZON2.md`](HORIZON2.md), [`FX_BANK.md`](FX_BANK.md)

---

## Standalone pivot summary

| Before (MURMUR v1.1.x) | After (extraction) |
|------------------------|-------------------|
| `EffectType::BinauralSpace` in master FX M3 | **Removed** from MURMUR; legacy type 11 → Reverb on load for Spatial bank |
| GLOBAL → QUASAR tab in PLAY UI | **Removed**; use QUASAR plugin editor |
| 18 Quasar mod-matrix destinations | **Removed** from MURMUR; mod in QUASAR plugin only (future) |
| Interstellar Spatial presets with embedded Quasar slot | Companion `.quasar` in `content/presets/quasar/interstellar/` + metadata `"spatial": "use-quasar-plugin"` |
| `BinauralSpaceProcessor` in `pw8_core` | **Retained** — linked by `pw8_quasar_plugin` |

**MURMUR keeps:** Reverb (M7 FDN), Tape delay, Vocoder, KOINS, synth — **not** binaural spatial.

---

## Historical plan (pre-pivot)

The sections below document the original **in-MURMUR** Quasar master-bus design (Phases 0–3, shipped v1.1.0–1.1.4). They remain useful for DSP architecture reference and Neuzeit research alignment.


## Executive summary (approval gate)

| Decision | Choice |
|----------|--------|
| **Slot type name** | `EffectType::BinauralSpace` (JSON/MCP: `"binaural_space"`, PLAY label: **QUASAR**) |
| **Summing point** | **Post-voice master bus only** — one global scene, not per-voice HRTF |
| **DSP approach** | Dual-path binaural panner (ITD/ILD + tiered HRTF) + embedded per-path room (reuse M7 FDN core) + CNTR dry anchor + post-sum stereo delay |
| **FDN reverb** | **Extend** existing `effects::Reverb` as internal room engine per QSR path; do **not** replace master Reverb slot |
| **Parameter count** | **47** user-facing params (see §3); **18** mod-matrix destinations at ship |
| **Phase 1 (approve now)** | Master FX `ModDestination` + Global-scope executor + SPACE macro → master reverb mix/size/decay |
| **Phase 2 milestone** | BinauralSpace bypass-safe stub → ITD/ILD + CNTR/QSR1/QSR2 routing audible |
| **CPU budget** | ~8–15% of one core @ 48 kHz / 128 buffer (Quality: Normal); High tier +25% |

---

## 1. Product vision

### What we are building

A new **master FX slot type** — **QUASAR** (`BinauralSpace`) — selectable alongside Saturation, Chorus, Delay, Reverb, EQ, Compressor, and Limiter in `Patch::masterEffects[4]`.

It is **not** a stereo widener or chorus substitute. It is a **headphone-first, deep binaural spatial mixer** inspired by Neuzeit Quasar's philosophy:

- **Dry center anchor (CNTR)** — mono-compatible low-mid body stays in the head
- **Two independent spatial layers (QSR1 / QSR2)** — height, azimuth, distance, per-path room
- **Scene-level motion** — global LFO/CV-style routing onto spatial + room params
- **Post-sum delay wash** — FW 2.0-style shared tail after spatial sum

### Placement in MURMUR signal flow

```
voices (Layer A [+ Layer B stack])
  → layer insert FX (3 slots)
  → stereo sum  (+ masterGain)
  → master FX chain (4 slots)   ← QUASAR lives here
  → DAW output
```

QUASAR occupies **one master slot** (typically slot 2 or 3). It replaces neither the dedicated **Reverb** slot nor the **Limiter** safety slot — it *composes* with them: Quasar = spatial scene; Reverb slot = additional algorithmic hall if desired.

### GLOBAL section in PLAY Advanced

PLAY Advanced gains a **GLOBAL** context (alongside OSC / FILTER / FX today):

| Sub-tab | Role |
|---------|------|
| **CHAIN** | Existing `FxChainStrip` — all 4 master slots + insert visibility |
| **QUASAR** | Deep editor when selected slot type = `BinauralSpace` — spherical scope, QSR1/2/CNTR balance, room, delay, ear profile |
| **OUTPUT** | Master gain, limiter peek, headphone/speaker mode toggle |

Basic / Compact: **no QUASAR knobs**. SPACE macro KOIN fans out to spatial/room params (see §6). Mission card shows `"SPACE → Quasar Mix, Distance, Room"` when patch metadata tags `masterFx: quasar`.

### Agentic patch metadata

```jsonc
"metadata": {
  "masterFx": "quasar",
  "performanceHints": [
    "headphone-first binaural wash",
    "CNTR holds bass — sweep SPACE for orbit"
  ]
}
```

Factory preset category tag: **`spatial`** (alongside `pad`, `bass`, `fx`).

---

## 2. DSP architecture

### 2.1 Design principles

1. **Global scene, not per-voice** — Quasar mixes two buses into one binaural scene; running HRTF on 32 voices is CPU-prohibitive and musically wrong.
2. **Reuse proven FDN** — Per-path room uses a **scaled-down instance** of the existing 8-line Householder FDN (`effects::Reverb`), not a new reverb algorithm.
3. **Tiered HRTF quality** — Ship ITD/ILD + simplified pinna filters at Normal; optional HRIR convolution at High (see §2.7).
4. **Latency honesty** — Report additional latency from HRTF crossfade buffers and post-sum delay line in `EffectLatency.hpp`.

### 2.2 Signal path (Quasar-complete)

```
                    ┌── HPF/LPF (route split) ──► [QSR1 spatial + room FDN] ──┐
 stereo in L/R ────┤                                                          ├──► sum ──► [post delay] ──► out L/R
                    ├── HPF/LPF ──► [QSR2 spatial + room FDN] ────────────────┤
                    └── HPF/LPF ──► [CNTR dry path] ──────────────────────────┘
```

**Per QSR path (QSR1, QSR2):**

| Stage | Algorithm | Notes |
|-------|-----------|-------|
| **Input conditioning** | Linkwitz-Riley 2-way or 3-band split | Optional; default HPF 120 Hz on QSR paths, CNTR HPF 80 Hz |
| **Distance** | Log gain + air absorption (1–6 kHz shelf) + optional Doppler | 20 cm – 10 m mapped to 0..1 param |
| **Position** | Spherical → ITD (interaural delay) + ILD (level/pinna EQ) | Azimuth 0–360°, height −1..+1 |
| **Early reflections** | 8–16 tap delay cluster, angle-dependent tap gains | From simplified image-source model |
| **Room** | Embedded FDN (4-line at Eco, 8-line at Normal+) | `roomAmount` = wet into FDN; shares `roomSize`, `roomDamping` |
| **Motion LFO** | Internal 3-target LFO (H/A/D) + external mod matrix | Quasar-style orbit without menu diving |

**CNTR path:**

- Band-limited dry stereo (optional HPF 100–200 Hz on QSR sends, not CNTR)
- `cntrLevel` vs `qsr1Level` / `qsr2Level` — Quasar balance knobs
- No HRTF — preserves mono fold-down and speaker translation

**Post-sum delay (FW 2.0 analog):**

- Stereo delay after QSR1+QSR2+CNTR sum
- Types: Tape (pitch on time change), Fade, Reverse (+ optional L/R flip)
- Time 3 ms – 20 s; feedback 0–100% (100% = freeze)
- BP filter on delay path (HPF + LPF width, 12 dB/oct)

### 2.3 Summing point: post-voice (confirmed)

| Option | Verdict |
|--------|---------|
| Per-voice binaural pan | **Reject** — 32× HRTF, breaks Quasar "two input" metaphor |
| Post-layer-insert, pre-master | **Reject** — bypasses master chain ordering user expects |
| **Post-voice master bus (one slot)** | **Accept** — matches Quasar hardware + MURMUR `masterEffects[]` |

Frequency-split routing (bass → CNTR, highs → QSR) is implemented **inside** the Quasar processor via input matrix HPF/LPF, not by patching separate insert EQ.

### 2.4 FDN reverb: extend vs new

| Approach | Decision |
|----------|----------|
| Replace M7 master Reverb slot | **No** — keep Bricasti-style FDN as independent effect |
| New room algorithm | **No** — duplicate R&D |
| **`ReverbProcessor` as sub-component** | **Yes** — extract `LateTank` + early cluster into `RoomEngine` callable from Quasar with scaled line count |

Refactor sketch:

```cpp
// effects/RoomEngine.hpp — shared by Reverb slot and BinauralSpace
class RoomEngine {
    void prepare(double sr, RoomQuality q);
    void process(float inL, float inR, const RoomParams& p, float& wetL, float& wetR);
};
```

Quasar embeds **two** `RoomEngine` instances (QSR1, QSR2) at 4–8 lines each; full Reverb slot keeps 8 lines + full M7 param surface.

### 2.5 Binaural core (ITD/ILD + HRTF)

**Normal tier (default):**

- ITD: max ±0.8 ms sin/law pan mapping from azimuth
- ILD: head shadow filter (one-pole + peak) per ear, azimuth-driven
- Height: spectral tilt + subtle ITD bias (elevation cues)
- Crossfeed: optional weak contralateral mix for speaker mode

**High tier:**

- Minimum-phase HRIR pair (48 taps) selected from **Ear Type** preset index (Human → Elephant mapping from Quasar manual — **original IRs, not copied**)
- Crossfade between ITD/ILD and HRIR via `hrtfBlend` 0..1

**Eco tier:**

- ITD + ILD only, 4-line room, no early reflections, no post delay

### 2.6 Headphone vs speaker compensation

| Mode | Behavior |
|------|----------|
| **Headphone** (default) | Full binaural ITD/ILD/HRTF; crossfeed off |
| **Speaker** | Reduce ITD 70%, widen ILD, add crossfeed 0.2–0.4, gentle MS widen on CNTR |
| **Auto** | Use host I/O hint if available; else Headphone |

Parameter: `outputMode` enum { Headphone, Speaker, Auto }.

### 2.7 CPU, latency, quality tiers

| Tier | Room lines | HRTF | Early refs | Post delay | Est. CPU* |
|------|------------|------|------------|------------|-----------|
| Eco | 4 × 2 paths | ITD/ILD | off | off | ~3% |
| Normal | 8 × 2 paths | ITD/ILD + pinna | 8 taps | on | ~8% |
| High | 8 × 2 paths | HRIR 48-tap | 16 taps | on + mod | ~15% |

*Single core, 48 kHz, 128-sample buffer, Apple M-series reference.

**Latency:**

| Source | Samples @ 48 kHz |
|--------|------------------|
| ITD buffer | ~40 (max ITD) |
| HRIR (High) | 48 |
| Post delay | user `delayTime` (reported via `getLatencySamples`) |
| Lookahead (none) | 0 |

Total added latency: **~2 ms** (Normal) + delay time; plugin `EffectLatency.hpp` updated.

---

## 3. Parameter surface (complete)

All params live on `EffectSlotParams` extension fields when `type == BinauralSpace`, plus shared `mix` and slot `targetIndex` for mod routes.

### 3.1 Bus balance (Quasar: QSR1 / QSR2 / CNTR)

| Param | Range | Default | Unit | Quasar manual |
|-------|-------|---------|------|---------------|
| `qsr1Level` | 0..1 | 0.65 | linear | QSR1 level |
| `qsr2Level` | 0..1 | 0.55 | linear | QSR2 level |
| `cntrLevel` | 0..1 | 0.85 | linear | CNTR level |
| `inputSplitHpfHz` | 20..500 | 120 | Hz | IN→QSR HPF |
| `cntrHpfHz` | 20..300 | 80 | Hz | CNTR HPF (mono bass) |

### 3.2 Spatial — QSR1

| Param | Range | Default | Unit | Quasar manual |
|-------|-------|---------|------|---------------|
| `qsr1Height` | −1..+1 | 0 | normalized | Height |
| `qsr1Angle` | 0..360 | 30 | degrees | Angle |
| `qsr1Distance` | 0..1 | 0.35 | 0=20 cm, 1=10 m | Distance |
| `qsr1LfoSpeed` | 0.01..20 | 0.25 | Hz | LFO speed |
| `qsr1LfoAmountHeight` | −1..+1 | 0 | % | LFO→Height |
| `qsr1LfoAmountAngle` | −1..+1 | 0 | % | LFO→Angle |
| `qsr1LfoAmountDistance` | −1..+1 | 0 | % | LFO→Distance |
| `qsr1AutoRotate` | 0..1 | 0 | 0=off, 1=on | Auto rotation |

### 3.3 Spatial — QSR2

Same as QSR1 with `qsr2*` prefix; defaults: angle 330°, distance 0.4.

### 3.4 Room (per QSR path)

| Param | Range | Default | Unit | Quasar manual |
|-------|-------|---------|------|---------------|
| `qsr1RoomAmount` | 0..1 | 0.45 | linear | Room amount |
| `qsr1RoomDamping` | 0..1 | 0.55 | HF decay | Room damping |
| `qsr1RoomSize` | 0.2..3 | 1.0 | size param | Room size |
| `qsr2RoomAmount` | 0..1 | 0.40 | linear | (QSR2) |
| `qsr2RoomDamping` | 0..1 | 0.50 | HF decay | |
| `qsr2RoomSize` | 0.2..3 | 1.1 | size param | |

### 3.5 Post-sum delay

| Param | Range | Default | Unit | Quasar FW 2.0 |
|-------|-------|---------|------|---------------|
| `quasarDelayType` | 0..2 | 0 | Tape/Fade/Reverse | Delay type |
| `quasarDelayTimeMs` | 3..20000 | 450 | ms | Time |
| `quasarDelaySync` | 0..1 | 0 | off/on | Clock sync |
| `quasarDelaySyncDiv` | 0..12 | 6 | 1/16..128× | Sync division |
| `quasarDelayFeedback` | 0..1 | 0.35 | 1=freeze | Feedback |
| `quasarDelayVolume` | 0..1 | 0.25 | linear | Delay volume |
| `quasarDelayHpfHz` | 20..2000 | 180 | Hz | BP low |
| `quasarDelayLpfHz` | 200..16000 | 4200 | Hz | BP high |

### 3.6 Global / master

| Param | Range | Default | Unit | Quasar manual |
|-------|-------|---------|------|---------------|
| `mix` | 0..1 | 1.0 | dry/wet | (slot mix) |
| `earType` | 0..5 | 0 | enum | Ear Type |
| `dopplerAmount` | 0..1 | 0 | linear | Distance→Pitch |
| `crossfeed` | 0..1 | 0 | linear | Speaker crossfeed |
| `outputMode` | 0..2 | 0 | Headphone/Speaker/Auto | — |
| `hrtfBlend` | 0..1 | 0 | ITD vs HRIR | (MURMUR High tier) |
| `qualityTier` | 0..2 | 1 | Eco/Normal/High | — |
| `bypass` | 0..1 | 0 | bool | Bypass |

**Total user-facing params: 47** (incl. shared `mix`).

### 3.7 Mod matrix destinations (`ModDestination` additions)

`targetIndex` = master slot index 0..3 for slot-specific params.

| Destination | Param | Scope | Default amount |
|-------------|-------|-------|----------------|
| `MasterFxMix` | `mix` | Global | 0.35 |
| `MasterReverbMix` | reverb `mix` (when type=Reverb) | Global | 0.35 |
| `MasterReverbSize` | `reverbSizeParam` | Global | 0.5 |
| `MasterReverbDecay` | `reverbDecaySeconds` | Global | 1.5 s |
| `MasterReverbPreDelay` | `reverbPreDelayMs` | Global | 15 ms |
| `MasterReverbDiffusion` | `reverbDiffusion` | Global | 0.25 |
| `MasterReverbModDepth` | `reverbModDepth` | Global | 0.2 |
| `MasterGain` | `voiceSettings.masterGain` | Global | 0.25 |
| **Phase 3 — Quasar** | | | |
| `QuasarQsr1Distance` | `qsr1Distance` | Global | 0.4 |
| `QuasarQsr2Distance` | `qsr2Distance` | Global | 0.4 |
| `QuasarQsr1Angle` | `qsr1Angle` | Global | 90° |
| `QuasarQsr2Angle` | `qsr2Angle` | Global | 90° |
| `QuasarQsr1Height` | `qsr1Height` | Global | 0.3 |
| `QuasarQsr2Height` | `qsr2Height` | Global | 0.3 |
| `QuasarRoomAmount` | qsr1+2 room avg | Global | 0.35 |
| `QuasarDelayFeedback` | `quasarDelayFeedback` | Global | 0.3 |
| `QuasarDelayTime` | `quasarDelayTimeMs` | Global | 200 ms |
| `QuasarCntrLevel` | `cntrLevel` | Global | 0.2 |

**Ship set: 18 destinations** (7 master reverb/gain Phase 1 + 11 Quasar Phase 3).

### 3.8 SPACE macro KOIN bundle (Spread)

| Destination | Sign | Weight | Quasar analog |
|-------------|------|--------|---------------|
| `QuasarRoomAmount` or `MasterReverbMix` | + | 1.0 | Room amount |
| `MasterReverbSize` / `qsr1RoomSize` | + | 0.7 | Room size |
| `MasterReverbDecay` | + | 0.5 | Tail |
| `MasterReverbPreDelay` | + | 0.25 | Distance cue |
| `QuasarQsr1Distance` + `QuasarQsr2Distance` | + | 0.4 | Distance |
| `QuasarCntrLevel` | − | 0.15 | Less CNTR as space opens |
| `Pan` (layer) | + | 0.2 | Optional width |

### 3.9 Morph KOIN keyframes (INTIMATE ↔ VOID)

| Keyframe | `position` | Overrides |
|----------|------------|-----------|
| **INTIMATE** | 0.0 | `qsr1Distance`: 0.1, `qsr2Distance`: 0.15, `qsr1RoomAmount`: 0.12, `cntrLevel`: 0.95, `masterEffects[2].mix`: 0.15 |
| **STAGE** | 0.45 | `qsr1Distance`: 0.4, `qsr2Distance`: 0.45, room amounts 0.35, `cntrLevel`: 0.75 |
| **VOID** | 1.0 | `qsr1Distance`: 0.85, `qsr2Distance`: 0.9, room amounts 0.72, `cntrLevel`: 0.45, `quasarDelayFeedback`: 0.55, `masterEffects[2].mix`: 0.8 |

`morphKoin` uses `paramOverrides` paths (Horizon 3 executor); metadata authoring starts Horizon 2.

---

## 4. UI / GLOBAL section

### 4.1 PLAY Advanced layout

```
┌─────────────────────────────────────────────────────────────┐
│  [OSC] [FILTER] [FX] [GLOBAL]                    Advanced ▾  │
├─────────────────────────────────────────────────────────────┤
│  GLOBAL · CHAIN │ QUASAR │ OUTPUT                            │
├─────────────────────────────────────────────────────────────┤
│  ┌─ chain flow (FxChainFlowView) ─────────────────────────┐  │
│  │ INS1 → INS2 → INS3 ══► M0 → M1 → [QUASAR] → M3       │  │
│  └────────────────────────────────────────────────────────┘  │
│  ┌─ QUASAR editor (when slot selected) ──────────────────┐  │
│  │  [spherical scope: QSR1 ●  QSR2 ●  listener ◎]         │  │
│  │  CNTR / QSR1 / QSR2 levels │ Height │ Angle │ Distance  │  │
│  │  Room: Amount │ Damping │ Size (per path tab)           │  │
│  │  Delay: Time │ Fdbk │ Freeze │ Type                      │  │
│  │  Ear Type │ Output Mode │ Mix                            │  │
│  └────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Basic / Compact

- **No QUASAR panel** — SPACE macro only
- Preset bar: `SPACE → Room, Size, Distance` via `spreadSummaryForMacro`
- Mission card: `"headphone-first — sweep SPACE for orbit"`

### 4.3 Wireframe / spectrum

- **Spherical scope** (QUASAR tab): top-down + side elevation; QSR dots orbit with LFO; CNTR dot fixed center
- Reuse FILTER tab FFT code path for **Room damping** audition (optional Phase 4)
- `FxWireframeView`: new case 11 — binaural ring + dual orbit paths

### 4.4 PLAY FX tab (existing)

When QUASAR not selected, FX tab unchanged. When master slot type = BinauralSpace, `FxEffectPlayParams` exposes 4 knobs:

| Knob 1 | Knob 2 | Knob 3 | Knob 4 |
|--------|--------|--------|--------|
| Distance | Angle | Room | Delay |

Full depth → GLOBAL → QUASAR sub-tab.

---

## 5. Patch schema & agentic

### 5.1 Schema

```jsonc
"masterEffects": [
  { "type": "bypass", "mix": 1.0 },
  { "type": "eq", "mix": 1.0, "eqLowGainDb": 2.0 },
  {
    "type": "binaural_space",
    "mix": 1.0,
    "qsr1Level": 0.65,
    "qsr2Level": 0.55,
    "cntrLevel": 0.85,
    "qsr1Height": 0.1,
    "qsr1Angle": 35.0,
    "qsr1Distance": 0.38,
    "qsr1RoomAmount": 0.42,
    "qsr1RoomSize": 1.05,
    "qsr1RoomDamping": 0.52,
    "qsr2Height": -0.05,
    "qsr2Angle": 325.0,
    "qsr2Distance": 0.42,
    "qsr2RoomAmount": 0.38,
    "qsr2RoomSize": 1.12,
    "qsr2RoomDamping": 0.48,
    "quasarDelayTimeMs": 520.0,
    "quasarDelayFeedback": 0.32,
    "quasarDelayVolume": 0.22,
    "earType": 0,
    "outputMode": 0,
    "qualityTier": 1
  },
  { "type": "limiter", "mix": 1.0, "limiterCeilingDb": -0.3 }
]
```

`EffectType` ordinal: **11** = `BinauralSpace`. JSON string: `"binaural_space"`.

### 5.2 MCP tools

**New:** `set_global_quasar_fx(patch_id, slot, params)`

```python
set_global_quasar_fx(
    patch_id="scratch",
    slot=2,
    params={
        "qsr1_distance": 0.4,
        "qsr2_distance": 0.45,
        "cntr_level": 0.8,
        "room_amount": 0.35,  # fans to qsr1+qsr2
        "delay_time_ms": 480,
    },
    metadata_tag="quasar",
)
```

**Extend:** `set_macro_koin` destinations:

```python
destinations=[
    {"destination": "master_reverb_mix", "target_index": 2, "amount": 0.35, "scope": "global"},
    {"destination": "master_reverb_size", "target_index": 2, "amount": 0.5, "scope": "global"},
    {"destination": "quasar_qsr1_distance", "target_index": 2, "amount": 0.4, "scope": "global"},
]
```

### 5.3 Example factory preset JSON

See [`content/presets/factory/Spatial/001-orbit-cathedral.pw8`] (Phase 4) — illustrative excerpt in §5.1 above.

---

## 6. Mod matrix / KOINS / morph integration

### 6.1 Global-scope executor (Phase 1)

```
Engine::process() — per sub-block (64 samples):
  1. Sum voices + layer inserts
  2. Build ModSourceValues (macros, MW, EXP, layer LFOs)
  3. MasterModOutputs = ModMatrixExecutor::applyMasterBus(routes, sources)
  4. masterEffects' = patch.masterEffects + offsets (clamp per param)
  5. sum *= masterGainMod
  6. masterChain_.process(masterEffects', sumL, sumR)
```

Routes to master destinations **must** use `scope: global`. Voice-scoped routes to master destinations are ignored (logged in debug).

### 6.2 SPACE macro default routes (Quasar preset template)

```jsonc
{ "source": 22, "destination": 13, "targetIndex": 2, "amount": 0.35, "scope": 2 },
{ "source": 22, "destination": 14, "targetIndex": 2, "amount": 0.5, "scope": 2 },
{ "source": 22, "destination": 15, "targetIndex": 2, "amount": 1.2, "scope": 2 }
```

(destination IDs illustrative — map to `MasterReverbMix`, `MasterReverbSize`, `MasterReverbDecay`)

### 6.3 Dissemination compatibility

| Param class | Dissemination? |
|-------------|----------------|
| Per-voice filter/WT/pan | **Yes** (existing MVP) |
| Master / Quasar spatial | **No** — scene is global by definition |
| Morph position | Horizon 3: optional per-note freeze (extend `macroDissemination` policy) |

Spatial params (`qsr1Angle`, etc.) are **never** disseminated — sweeping SPACE affects the whole mix while held notes keep their per-voice BLOOM snapshot.

---

## 7. Phased implementation roadmap

| Phase | Scope | Effort | Milestone |
|-------|-------|--------|-----------|
| **0** | This plan doc + research cross-links | **S** | Curtis approval |
| **1** | `ModDestination` master reverb/gain; Global executor in `Engine`; MCP + MacroSpread labels; SPACE routes on Interstellar pad | **M** | Macro2 moves master reverb in Logic without Advanced FX |
| **2** | `EffectType::BinauralSpace`; `RoomEngine` extract; ITD/ILD panner; CNTR/QSR1/QSR2 routing; APVTS + PLAY 4-knob surface | **L** | **Shipped 2026-08-14** — headphone orbit audible |
| **3** | Full 47-param surface; embedded room FDN per path; early reflections; post-sum delay + freeze; 18 mod destinations; `set_global_quasar_fx` MCP; GLOBAL → QUASAR UI tab | **L** | Feature-complete vs Quasar *philosophy* |
| **4** | High-tier HRIR; spherical scope UI; factory `spatial/` presets; morph keyframes; QA soak + reference track A/B | **M** | Ship candidate |

### Phase 2 deliverables (shipped 2026-08-14)

- [x] `RoomEngine.hpp` — 8-line embedded FDN for QSR paths
- [x] `BinauralPanner.hpp` — ITD/ILD + distance/height cues
- [x] `BinauralSpaceProcessor` — CNTR/QSR1/QSR2 routing + post-sum stereo delay
- [x] Wired into `EffectChain` (no passthrough)
- [x] 20 Quasar APVTS fields on `EffectSlotParams` + serializer
- [x] PLAY FX: 4-knob QUASAR surface + expanded CNTR/Q2/Room/Delay row
- [x] `MasterReverb*` mod routes map to Quasar room params when type=BinauralSpace
- [x] Interstellar showcase presets: `001-cathedral-nebula`, `004-tesseract-bloom`, `092-echo-chamber`
- [x] Unit tests: ITD panner + BinauralSpace stereo width + bypass

---

### Phase 1 deliverables (shipped)

- [x] `ModDestination`: `MasterFxMix`, `MasterReverbMix`, `MasterReverbSize`, `MasterReverbDecay`, `MasterReverbPreDelay`, `MasterReverbDiffusion`, `MasterReverbModDepth`, `MasterGain`
- [x] `ModMatrixExecutor::applyMasterBus()` + `Engine` hook
- [x] MCP `patch_schema.py` destination strings
- [x] `MacroSpread` short labels
- [x] Interstellar preset SPACE routes (Phase 2 showcase presets)

---

## 8. Sound design & QA

### 8.1 Reference tracks

| Reference | Use |
|-----------|-----|
| Neuzeit Quasar factory presets (manual audio examples) | Spatial motion + room balance |
| Valhalla VintageVerb / Room | Reverb quality baseline (master Reverb slot, not Quasar) |
| `Interstellar/001-cathedral-nebula` | SPACE macro integration |
| Headphone check: Björk "Headphones" mix, Holly Herndon spatial EP | Externalization |

### 8.2 A/B protocol

1. **Quasar module recording** (hardware or demo) vs MURMUR QUASAR — same source stem
2. Score 1–5: **Externalization**, **Motion smoothness**, **Low-end mono**, **CPU**, **Mix clarity**
3. Pass: ≥4 on externalization vs stereo chorus widener; ≥3 vs Quasar hardware (expect gap until Phase 4 HRIR)

### 8.3 Headphone checklist

- [ ] Low end mono-stable (CNTR holds <150 Hz)
- [ ] No ping-pong phasing when QSR1/QSR2 angles converge
- [ ] ITD clicks absent on slow angle automation
- [ ] Freeze (delay feedback = 100%) stable 60 s
- [ ] Speaker mode reduces in-head localization vs Headphone mode
- [ ] No NaN/inf after 4 h soak (`tests/regression/RenderSanityTests`)

### 8.4 Logic soak test plan

1. Build AU; load `001-orbit-cathedral` (Phase 4 preset)
2. Hold chord; automate Macro2 (SPACE) — verify master reverb/spatial move (Phase 1)
3. Automate `qsr1Angle` via MCP param path (Phase 3)
4. Export 5 min; scan for clicks (manual + `pw8-render` metrics)
5. CPU meter: Normal tier <15% on M1/M2

---

## 9. What we are NOT building

| Out of scope | Reason |
|--------------|--------|
| Full Quasar hardware clone | Legal/technical; inspiration only |
| 16 HP Eurorack UI / LED rings / QSR menu tree | Conflicts with Obsidian KOINS identity |
| Quasar HRTF table port | Original IRs only |
| Per-voice binaural on 32 voices | CPU + wrong metaphor |
| Multi-speaker Atmos/5.1 | Headphone-first; speaker mode is enhanced stereo |
| Replacing M7 FDN master Reverb | Complementary slots |
| DESIGN Mode FX editor | PLAY Advanced is the deep editor (`UI_DIFFERENTIATION_BRIEF.md`) |
| 15 reverb params as Basic KOINS | Violates 1–3 KOINS policy |

---

## Appendix A — File touch list (Phase 2+)

| Area | Files |
|------|-------|
| Effect type | `EffectTypes.hpp`, `EffectChain.hpp`, `BinauralSpace.hpp` (new), `RoomEngine.hpp` (new) |
| APVTS | `PluginState.cpp` (+47 fields → `kNumEffectSlotFields`), `PatchworkEightProcessor.cpp` |
| Serializer | `PatchSerializer.cpp`, `PATCH_FORMAT.md` |
| PLAY UI | `FxEffectPlayParams.h`, `FxChainStrip.cpp`, `GlobalPanel` (new), `PlayModeEditor.cpp` |
| Mod | `ModMatrixTypes.hpp`, `ModMatrixExecutor.hpp`, `Engine.cpp`, `ModRoutingUi.cpp` |
| MCP | `patch_schema.py`, `patch_builder.py`, `server.py` |
| Tests | `EffectsTests.cpp`, `ModMatrixTests.cpp`, `RenderSanityTests.cpp` |
| Presets | `content/presets/factory/Spatial/*.pw8`, `generate_factory_presets.py` |

---

## Appendix B — Curtis approval checklist

- [ ] Approve **post-voice global** summing (not per-voice)
- [ ] Approve **BinauralSpace** name + **QUASAR** PLAY label
- [ ] Approve **Phase 1** merge (master mod matrix) before Phase 2 DSP
- [ ] Approve **47-param** depth (hidden behind SPACE in Basic)
- [ ] Confirm factory tag **`spatial`** category

---

## Cross-links

| Doc | Relevance |
|-----|-----------|
| [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md) | Source research |
| [`MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md) | INTIMATE ↔ VOID keyframes |
| [`HORIZON2.md`](HORIZON2.md) | KOINS/MCP shipped scope |
| [`FX_BANK.md`](FX_BANK.md) | Master chain + FDN reverb |
| [`ASM_MACRO_KOINS_RESEARCH.md`](ASM_MACRO_KOINS_RESEARCH.md) | SPACE macro bundles |
