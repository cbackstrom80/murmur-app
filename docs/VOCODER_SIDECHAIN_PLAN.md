# Vocoder + Sidechain — DEEP CYCLE Plan

**Status:** **MVP shipped v1.1.4** (8-band sidechain vocoder FX slot)  
**Related:** [`EXT_OSCILLATOR_AU_THEORY.md`](EXT_OSCILLATOR_AU_THEORY.md), [`FX_BANK.md`](FX_BANK.md)

---

## Architecture (DEEP CYCLE)

```
Logic Vocal BUS ──send──► Sidechain bus ──AU picker──► MURMUR Sidechain input
                                                              │
                    ┌─────────────────────────────────────────┤
                    │                                         │
                    ▼                                         ▼
         SidechainFollower (RMS)                    Vocoder modulator bands
         → ModSource::Sidechain                     (per-band envelope followers)
         → mod matrix destinations                         │
                    │                                         │
                    ▼                                         ▼
              Quasar / filter / etc.              Carrier = synth bus (insert or master slot)
```

**Two sidechain consumers share one AU input:**

1. **Mod matrix** — scalar envelope 0..1 (`ModSource::Sidechain`), unchanged from v1.1.0.
2. **Vocoder FX** — full-band audio modulator (v1.1.4), per-sample aligned with engine render.

---

## Shipped MVP (v1.1.4)

| Piece | Where |
|-------|--------|
| `EffectType::Vocoder` (12) | `engine/include/pw8/effects/EffectTypes.hpp` |
| 8–16 band vocoder DSP | `engine/include/pw8/effects/Vocoder.hpp` |
| Sidechain per-sample into FX chain | `Engine::process(..., sidechainL, sidechainR)` |
| AU sidechain → engine | `PatchworkEightProcessor::processBlock` |
| PLAY FX UI | TYPE chip **VOCODER**, knobs: Bands, Formant, Sibilance, SC Gain + Mix |
| Tests | `EffectsTests.cpp` — sidechain gates carrier, mix=0 dry pass |

### Signal flow

1. Voices sum → layer insert chain (slots 0–2) → optional stack layer B → master chain (slots 0–3).
2. Any slot with `type == Vocoder`:
   - **Carrier** = slot input (synth audio at that point in the chain).
   - **Modulator** = AU sidechain stereo (mono sum). If sidechain silent (standalone / no bus), falls back to carrier mono (self-mod test).
3. Each band: bandpass → envelope follower on modulator band → multiply carrier band → sum → mix.

### APVTS param mapping (Vocoder type)

Reuses existing scalar fields (no new APVTS IDs in MVP):

| PLAY label | APVTS field | Range / meaning |
|------------|-------------|-----------------|
| Mix | `Mix` | 0..1 dry/wet |
| Bands | `FreqShiftLowCutHz` | 8..16 (stored as Hz field, interpreted as band count) |
| Formant | `FractalMorph` | 0..1 → formant multiplier ~0.5×..2× |
| Sibilance | `FreqShiftHz` | 0..2000 → 0..1 high-band envelope boost |
| SC Gain | `SaturationDrive` | 0..48 dB → 0..2 modulator gain |

**Phase 2:** dedicated `vocoder*` JSON/APVTS fields; mod-matrix destinations for mix/formant.

---

## Logic setup (vocoder + sidechain mod)

1. Create **Vocal BUS** — send from vocal track(s); aux output muted if sidechain-only.
2. Insert **MURMUR** on instrument track.
3. Plugin header **Side Chain** menu → pick Vocal BUS.
4. **FX tab (PLAY)** — pick insert or master slot → TYPE **VOCODER** → set Bands / Formant / Mix.
5. Optional: **MOD tab** — route **SIDECHAIN →** Quasar distance (or any dest) for envelope ducking alongside vocoder.

Performance badge: `Sidechain (AU) — route bus for MOD and/or VOCODER FX` when bus available.

---

## Phase 2 (not in MVP)

| Item | Notes |
|------|-------|
| Dedicated APVTS + patch JSON fields | Clean preset interchange |
| Mod matrix → vocoder mix / formant | Needs `ModDestination` entries |
| 16-band minimum on iPad | CPU budget study |
| EXT operator (replace op 0 with bus audio) | See `EXT_OSCILLATOR_AU_THEORY.md` |
| VST3 aux sidechain parity | `getPluginHasMainInput() → false` pattern |
| Formant preserve / unvoiced detection | Sibilance upgrade |

---

## CPU / quality notes

- 8 bands default; 16 bands ≈ 2× biquad + envelope cost per vocoder slot.
- Log-spaced centers 100 Hz – 8 kHz; Q ≈ 4.5.
- One vocoder on master slot is the intended DEEP CYCLE sweet spot (full mix as carrier).
