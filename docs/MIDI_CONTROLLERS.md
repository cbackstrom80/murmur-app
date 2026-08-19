# MIDI Controllers (Logic Pro + MURMUR)

Quick reference for sound-design performance in Logic Pro with MURMUR / MURMUR.

## Standard factory MIDI layout

Every factory and showcase preset follows this minimum layout (applied by `ensure_standard_midi_layout()` in `scripts/generate_factory_presets.py` and at runtime via `PatchModDefaults.hpp` for mod wheel and expression fallbacks).

| Requirement | Source | Ordinal | Typical destination |
|-------------|--------|---------|---------------------|
| Mod wheel | CC1 | 29 (`ModWheel`) | Filter cutoff (category-tuned depth) |
| Velocity | Note vel | 17 (`Velocity`) | Filter cutoff (bass/lead/pad/ambient) or op level (seq) |
| Primary macro | Macro 1–8 | 21–28 (`Macro1`…) | Category primary macro → filter or WT position |
| Channel aftertouch | Ch pressure | 18 (`ChannelPressure`) | Filter cutoff — **pads & ambient only** |
| Brightness / slide | CC74 | 20 (`MpeSlide`) | Filter cutoff — **all categories** |
| Expression | CC11 | 30 (`Expression`) | Filter resonance (bass/lead/seq) or op level (pad/ambient) |

### Category-specific defaults

| Category | Mod wheel depth | Primary macro | Macro destination | Extra routes |
|----------|-----------------|---------------|-------------------|--------------|
| Bass | +24 st → cutoff | Macro 1 (PUNCH) | Filter cutoff | Expression → resonance |
| Lead | +28 st → cutoff | Macro 1 (EDGE) | Filter cutoff | CC74 → cutoff (+14 st), Expression |
| Pad | +18 st → cutoff | Macro 1 (WARMTH) | Op 1 WT position | Aftertouch → cutoff, Expression → level |
| Seq | +20 st → cutoff | Macro 1 (RATE) | Filter cutoff | Velocity → op level, Expression |
| Ambient | +16 st → cutoff | Macro 6 (SPACE) | Filter cutoff | Aftertouch → cutoff, Expression → level |

### Feature macro KOINS policy

- **Range:** 1–3 feature macro KOINS per patch in Basic/Compact PLAY (`kMaxFeatureKoinCount = 3`).
- **Semantic model:** each KOIN is a **macro** (Macro1–8) with active `modRoutes` — contextual performance bundles, not raw APVTS params.
- **Authored:** `uiFocus.knobs` with `kind: macro` and indices 0–2 typical; `maxKnobs` default 3 (clamped 1–3).
- **Runtime fallback:** `inferPatchFocusKnobs()` picks routed macros (Macro1–3 first); no param padding.
- Factory presets wire 2–4 mod routes per featured macro with patch-specific names (PUNCH, BLOOM, SPACE, …).

## Mod Wheel (CC1)

**Status: implemented.**

- CC1 is parsed from incoming MIDI and stored per channel (0..1).
- Every patch gets a default **Mod Wheel → Global Filter Cutoff** route (+24 semitones at full wheel) on load if none is authored.
- Factory presets additionally store an explicit mod-wheel route in `.pw8` (see table above).
- Route as a mod-matrix source: `ModSource::ModWheel` (ordinal **29** in `.pw8` JSON).
- **PLAY screen badge:** Knobs of Interest panel shows live wheel position and destination, e.g. `Mod Wheel (CC1)  64%  ->  GLOBAL FILTER CUTOFF  (+24.0 st max)`.
- **APVTS parameter:** `modWheel` — mirrored from MIDI for host parameter lists / Smart Control readout (MIDI-driven; not written back to the engine).

### Logic Pro tips (Ben)

1. **Mod wheel in every patch:** Move the mod wheel while holding a note — filter should brighten/darken on all factory and init patches.
2. **Smart Controls:** Full 8-knob template in [`LOGIC_SMART_CONTROLS.md`](LOGIC_SMART_CONTROLS.md) — macros, mod wheel, and expression.
3. **Custom routing:** Open **Mod Matrix (M)** → click **MW** chip → click **Cutoff** (or drag MW onto a ringed knob). Amount is editable in the connections list.
4. **MIDI FX:** Logic’s “Modulator” MIDI FX can generate CC1 if your controller lacks a wheel.

## Kawai MP11SE performance CC map

**Status: implemented** (see [`KAWAI_MP11SE.md`](KAWAI_MP11SE.md) for Ben’s step-by-step keyboard setup).

Hardware CCs are applied in `PerformanceMidiMap.hpp` **before** APVTS values are pushed to the engine, so MP11SE knobs move the same macros shown in Knobs of Interest:

| CC | GM name | APVTS target | Mod matrix |
|----|---------|--------------|------------|
| 7 | Volume | Master gain | — |
| 11 | Expression | Macro 2 | `Expression` (30) |
| 74 | Brightness | Macro 3 | `MpeSlide` (20) |
| 71 | Harmonic | Macro 4 | — |
| 73 | Attack | Macro 5 | — |
| 91 | Reverb send | Macro 6 | — |
| 93 | Chorus send | Macro 7 | — |

CC1 (mod wheel) is **not** remapped to a macro — it uses the dedicated mod-wheel path and `modWheel` APVTS readout.

## Expression (CC11)

**Status: implemented.**

- CC11 is parsed per channel (0..1) and latched on active voices (same pattern as mod wheel).
- Default route on load: **Expression → Filter Resonance** (+0.35 at full pedal) if the patch has no expression route.
- Factory presets store category-tuned expression routes (resonance for bass/lead/seq; operator level for pads/ambient).
- Mod matrix source: `ModSource::Expression` (ordinal **30**).
- Also drives **Macro 2** via performance CC map (pedal can move both resonance modulation and the macro-mapped parameter).
- **APVTS parameter:** `expression` — mirrored from MIDI for host parameter lists / Smart Controls (MIDI-driven; not written back to the engine).
- **PLAY screen badge:** Knobs of Interest shows live pedal position and destination, e.g. `Expression (CC11)  72%  ->  FILTER RESONANCE  (+0.35 max)`.

## Other controllers

| Controller | MIDI | Engine behavior | Mod matrix source |
|------------|------|-----------------|-------------------|
| Pitch bend | PB | ±2 semitones pitch (channel-wide) | — (direct pitch) |
| Aftertouch (channel) | Ch pressure | Per-note on channel | `ChannelPressure` (18) |
| Poly aftertouch | Poly AT | Per note | `PolyAftertouch` (19) |
| MPE slide / timbre | CC74 | Per-note slide + performance CC → Macro 3 | `MpeSlide` (20) |
| Sustain | CC64 | Hold notes | — |
| Brightness (GM) | CC74 | Same as MPE slide when used as CC74 | `MpeSlide` (20) |

## Macros vs mod wheel

- **Macros 1–8:** Host-automatable APVTS parameters — ideal for Logic Smart Controls, knobs, and automation lanes. Factory patches wire at least one macro into `modRoutes` and surface it in `uiFocus`.
- **Mod wheel:** Performance MIDI only — use mod matrix to route to filter, resonance, operator level, pan, or wavetable position.

## MIDI learn

**Not implemented yet.** Assignment today:

1. Drag a mod source chip onto a ringed knob, or
2. Arm a source → click Cutoff/Resonance in the Route Editor, or
3. Edit `modRoutes` in a `.pw8` preset file, or
4. Regenerate factory content with `python3 scripts/generate_factory_presets.py`.

## AU / Logic parameter exposure

All automatable scalars (802+) including macros and `modWheel` are exposed via JUCE APVTS for AU and VST3. Mod routes themselves are patch data (saved in `.pw8` / plugin state), not individual automatable parameters.
