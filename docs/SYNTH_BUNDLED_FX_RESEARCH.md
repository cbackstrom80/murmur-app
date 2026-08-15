# Synth Bundled FX Research — MURMUR Ship Strategy

**Date:** 2026-08-14  
**Branch:** `cursor/favorites-unison-stack-daw`  
**Context:** Quasar binaural spatial is pivoting to a **standalone plugin** (see [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) pivot). This document defines what FX **MURMUR the synth** should ship built-in vs what ships separately.  
**Audience:** Curtis — product gate for MURMUR v1.2+ FX scope.

---

## Executive summary (Curtis)

| Decision | Recommendation |
|----------|----------------|
| **Core ship list (v1.2+)** | **EQ, Compressor, Limiter, Chorus, Tape Delay, Reverb (M7 FDN + character presets), Saturation** — performance-first, PLAY Advanced deep editors |
| **Sound-design extras (keep, not KOINS)** | **NodeDelay, FreqShiftEcho, FractalEcho** — agent/sound-design tier; not factory-default chains |
| **Specialty (keep, narrow use)** | **Vocoder** — DEEP CYCLE / sidechain feature; not a default insert |
| **Extract to separate product** | **BinauralSpace / QUASAR** — headphone-first spatial scene mixer; too deep and CPU-heavy for bundled synth FX |
| **Do not add to MURMUR** | Convolution reverb, mastering suite clones, guitar-pedal library, multiband dynamics, shimmer/spring as separate engines (use Reverb character bundles first) |
| **Slot architecture** | **Keep 3 insert + 4 master** — matches industry sweet spot; Quasar removal frees one master slot for Reverb/EQ/Comp/Limiter defaults |
| **KOINS** | **BLOOM** → tone/body (filter, WT, comp makeup); **SPACE** → time/ambience (reverb mix/size/decay, delay mix/feedback, chorus depth) — no Quasar params after extraction |

---

## 1. Industry baseline

### 1.1 Comparison table

| Synth / platform | Bundled FX (built-in, not separate plugins) | Slot / routing model | Performance vs sound-design split |
|------------------|-----------------------------------------------|----------------------|----------------------------------|
| **Serum 2** | Chorus, Phaser, Flanger, Distortion (+ Overdrive), EQ, Compressor, Reverb (+ Vintage/Nitrous/Basin types), Delay (HQ), Hyper/Dimension, **Bode freq shifter**, **Convolve (IR)**, Utility, **splitters** (L/H, L/M/H, M/S) | **2 independent FX buses**, freely arrangeable; duplicate any module | **Performance:** default chains (Chorus→EQ→Delay→Reverb). **Sound-design:** Convolve, Bode, stacked distortions, M/S splits |
| **Vital** | Chorus, Compressor, Delay, Distortion, EQ, Filter, Flanger, Phaser, Reverb | **9 reorderable slots**, single serial chain | **Performance:** Reverb + Chorus + Comp punch. **Sound-design:** Filter in FX, distortion modes, deep matrix on FX params |
| **Pigments 5** | Delay, Tape Echo, PS Delay, Reverb, Shimmer, Compressor, Multiband, Multi Filter, Param EQ, Distortion, BitCrusher, Super Unison, Chorus (×2 flavors), Flanger (×2), Phaser, Stereo Pan, Vocoder (v5+) | **3 buses × 3 slots** (FX A, FX B, Aux send/return); filter routing to buses | **Performance:** bus A = tone (Comp/EQ), bus B = space (Delay/Reverb). **Sound-design:** Shimmer, PS Delay, Multiband, BitCrusher |
| **Omnisphere 3** | **93 FX units** (103 w/ extensions): reverbs (Super Verb, Velvet, Shimmer), delays (Magnetic, Refraction, Backward), dynamics (1176, opto, multiband), EQ (console emulations), distortion, modulation, creative (Warp Shifter, Ring of Fire), tape/console color | **53 FX racks** (4 slots/rack) at layer/patch/multi level; also **standalone Omnisphere FX** plugin | **Performance:** patch COMMON rack = Comp + EQ + Reverb. **Sound-design:** entire library — synth is also an FX host |
| **Massive X** | **Insert (3):** Anima, Bass Enhancer, Bitcrush, Freq Shifter, Ring Mod, S&H, Track Delay, PM/Insert Osc. **Stereo (3):** Reverb, Stereo Delay, Quad Chorus, Flanger, EQ, Multi Compressor, Nonlinear Lab, Dimension/Stereo Expander | 3 insert (per-voice) + 3 stereo (post-sum); series/parallel/sum routing | **Performance:** Stereo Delay + Reverb + Dimension. **Insert FX = sound-generation** (not traditional mix FX) |
| **Diva (u-he)** | Chorus (3 modes), Phaser (2), Plate reverb, Delay, Rotary — **5 types, 2 slots** | **2 serial slots**, pick one type each | **Performance-first:** minimal vintage set; no conv/multiband |
| **Zebra2 (u-he)** | ModFX grid: Chorus, EQ, Reverb (NuRev), Phaser, etc. — modular **per-slot** | Small effects grid on GLOBAL page | Sound-design synth: FX grid is deep but not a “mastering chain” |
| **Phase Plant** | **34+ Kilohearts Essentials Snapins** (Delay, Reverb, Chorus, Flanger, Phaser, Comp, Limiter, EQ, Distortion, Bitcrush, Freq Shifter, Transient Shaper, Haas, …) in **3 lanes** | 3 lanes, serial or parallel; **polyphonic FX** option | **Performance:** lane 3 = Reverb/Delay. **Sound-design:** unlimited Snapin stacks, Multipass-style (premium) |
| **Hydrasynth** | **Pre-FX:** Chorus, Flanger, Rotary, Phaser, Lo-Fi, Tremolo, EQ, Comp (sidechain), Distortion. **Delay** (5 types). **Reverb** (Hall/Room/Plate/Cloud + freeze). **Post-FX:** same set as pre | Pre → Delay → Reverb → Post (hardware-style) | **Performance:** one block each; BPM sync delay. **Sound-design:** Pre+Post same menu = flexible tone vs space |
| **Waldorf Iridium** | **5 serial insert FX** (Chorus, Flanger, Delay, Reverb, Phaser, …) + **Digital Former** (Drive, Bitcrush, Comb, RingMod — voice-level) | 5 effect units, serial | **Performance:** Delay + Reverb at end. Former = synthesis color |
| **Arturia Jun-6 V** | **Chorus** (3-mode BBD, always on), **Delay**, **Reverb** (Advanced panel, off by default) | Chorus in main; Delay/Reverb in Advanced | **Performance:** chorus + fader-up reverb. Juno identity = chorus |
| **Arturia Jup-8 V** | **Voice FX (2):** distortion/phaser/chorus in chain. **Patch FX (2):** Delay, Reverb, Flanger, Phaser | Voice FX in synthesis path; Patch FX post-VCA | **Performance:** patch-level Delay + Reverb. Voice FX = character |

### 1.2 Pattern summary

| Tier | What ships | Examples |
|------|------------|----------|
| **Tier 0 — every serious synth** | EQ, dynamics (comp and/or limiter), modulation (chorus/flanger/phaser ≥1), delay, reverb | Vital, Diva, Hydrasynth, Serum |
| **Tier 1 — modern wavetable / sound-design** | Distortion/saturation, stereo width, tempo-sync delay, reverb type/character switch | Serum 2, Pigments, Hydrasynth |
| **Tier 2 — deep design (optional in synth)** | Freq shift, bitcrush, multiband, shimmer, convolution, vocoder | Serum Convolve, Pigments Shimmer/Multiband, MURMUR NodeDelay/FractalEcho |
| **Tier 3 — ship separately** | Binaural/HRTF spatial, full mastering suite, IR convolution library, guitar pedal clones | Quasar → standalone; Omnisphere FX as separate plugin; conv reverb as optional |

**MURMUR today (pre-extraction)** sits between Tier 1 and Tier 2: strong delay/reverb/dynamics core **plus** experimental delays **plus** Quasar (Tier 3 — wrong place).

---

## 2. Categories every synth needs

These are **non-negotiable** for a performance-first synth that must sound finished inside the preset:

| Category | Why | Industry norm | MURMUR status |
|----------|-----|---------------|---------------|
| **EQ** | Tone shaping before/after dynamics and space; fix mud/harshness in patch | 3-band parametric (Vital, Serum, Hydrasynth) | ✅ `Eq` — keep |
| **Compression** | Punch, sustain, “produced” factory sound; glue on master | Vital, Serum, Pigments, Hydrasynth pre/post | ✅ `Compressor` — **deepen** (VCA/FET/Opto per [`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md)) |
| **Limiter / safety** | Prevent hot patches from clipping DAW; live performance | Often implicit (Serum soft clip); explicit limiter rare but valuable | ✅ `Limiter` — keep on **master slot 4** always |
| **Chorus / flanger / phaser** | Width and motion without spatial CPU; Juno identity | ≥1 mod effect (Vital: 3; Diva: 2 types in 2 slots) | ✅ `Chorus` — **add Flanger/Phaser later only if one slot type enum**, not three separate products |
| **Delay** | Rhythm, depth, slap; tempo sync expected in 2026 | Tape/analog stereo delay (Serum, Vital, Hydrasynth) | ✅ `TapeDelay` + tempo sync — **default insert delay** |
| **Reverb** | Ambience completes pad/lead presets; must be musical at moderate CPU | 1 strong algorithm + character switch (Hydrasynth 4 types; Serum 4; Pigments Reverb+Shimmer) | ✅ M7 FDN — **add character bundles** (Plate/Hall/Room/Spring) per deep pass |

**Default factory master chain (post-Quasar):**

```
M1: Reverb  →  M2: EQ  →  M3: Compressor  →  M4: Limiter
```

**Default factory insert chain:**

```
I1: Saturation (light)  →  I2: Chorus  →  I3: Tape Delay (sync optional)
```

---

## 3. Categories nice-to-have

Worth keeping in the **algorithm bank** but not required for every preset or PLAY Basic surface:

| Category | Ship in MURMUR? | Notes |
|----------|-----------------|-------|
| **Distortion / saturation** | ✅ Already (`Saturation`) | Serum/Pigments/Hydrasynth all ship it; keep as insert color |
| **Vocoder** | ✅ Keep, niche | Pigments added vocoder; MURMUR MVP shipped ([`VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md)). Master slot or insert — not default |
| **Stereo width / Haas / dimension** | ⚠️ Defer | Serum Hyper/Dimension, Massive Dimension Expander — **Chorus covers 80%**; Quasar standalone covers headphone width |
| **Multiband comp / dynamics** | ❌ Not v1.2 | Pigments/Omnisphere — mastering territory |
| **Transient shaper** | ❌ Defer | Kilohearts Snapin; low priority vs comp character |
| **Bitcrush / lo-fi** | ⚠️ Optional later | Hydrasynth Lo-Fi; Vital distortion includes crush — could extend `Saturation` modes before new type |
| **Shimmer / spring reverb** | ⚠️ Via Reverb character | Pigments separate Shimmer; prefer `ReverbCharacter` enum over new slot type ([`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md) §C) |
| **Experimental delays** | ✅ Keep in bank | NodeDelay, FreqShiftEcho, FractalEcho — **MURMUR differentiator**; agent/sound-design presets only |
| **Freq shifter** | ✅ Via FreqShiftEcho | Serum 2 Bode; Massive X insert — covered |

---

## 4. Categories to EXCLUDE from synth (ship separately)

| Category | Why exclude | MURMUR action |
|----------|-------------|---------------|
| **Binaural / HRTF spatial (Quasar)** | CPU-heavy, headphone-first, 47 params, scene-mixer metaphor — not “finish the patch” FX | **Extract to standalone Quasar plugin**; remove `BinauralSpace` from MURMUR slot picker v1.2+ |
| **Heavy convolution reverb** | IR load, latency, disk/browser — Serum ships it but it’s optional depth; conflicts with M7 FDN identity | Do not add; link users to DAW conv reverb or future Patchwork FX product |
| **Mastering suite** (multiband comp, brickwall maximizer, console EQ emulations) | Omnisphere/Pigments overlap with Ozone/Pro-L — wrong job for synth master bus | MURMUR limiter = safety only; comp = musical glue |
| **Guitar pedal clone library** | Arturia/Pigments mine this for color — scope creep for FM/wavetable synth | Saturation + comp transformer color sufficient |
| **Full modular FX host** (Phase Plant / Omnisphere scale) | 93 units / 34 Snapins — different product category | MURMUR: **10–12 curated algorithms**, not a host |
| **Per-voice binaural / Atmos** | CPU + wrong metaphor ([`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) §2.3) | Quasar standalone handles spatial; MURMUR keeps stereo pan |

---

## 5. MURMUR recommendation — v1.2+ after Quasar extraction

### 5.1 Minimum viable FX rack (performance-first, PLAY Advanced)

**Ship and factory-default these 8 slot types:**

| Type | Role | PLAY surface |
|------|------|--------------|
| `Eq` | Tone | 3-band rows in FX tab |
| `Compressor` | Glue / punch | Threshold, ratio, knee, GR meter; character enum |
| `Limiter` | Safety | Ceiling + lookahead; master M4 locked suggestion |
| `Chorus` | Width | Rate, depth, mix |
| `TapeDelay` | Rhythm / space | SYNC/DIV (shipped v1.1.2), feedback, duck |
| `Reverb` | Ambience | M7 params + **character** chip (Plate/Hall/Room/Spring) |
| `Saturation` | Harmonics | Drive; light insert default |
| `Bypass` | — | — |

**Keep in bank, not factory-default:**

| Type | Role |
|------|------|
| `NodeDelay` | Sound-design multitap tree |
| `FreqShiftEcho` | Inharmonic echo |
| `FractalEcho` | Procedural morph delay |
| `Vocoder` | Sidechain DEEP CYCLE |

**Remove from MURMUR (standalone Quasar product):**

| Type | Migration |
|------|-----------|
| `BinauralSpace` | Presets with `metadata.masterFx: quasar` → tag points to Quasar plugin; MURMUR preset loads with M1 Reverb + note in mission card |

### 5.2 What to deepen (v1.2–1.4)

From [`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md):

| Priority | Work | Version |
|----------|------|---------|
| **P0** | Remove Quasar from MURMUR slot types; migrate Spatial factory presets | **1.2.0** |
| **P0** | Compressor: VCA/FET/Opto character, GR meter, auto-makeup, mod-matrix destinations | **1.2.0** |
| **P1** | Reverb: `ReverbCharacter` preset bundles on existing FDN (Plate/Hall/Room/Spring) | **1.3.0** |
| **P1** | Delay sync on NodeDelay / FractalEcho / FreqShiftEcho | **1.3.0** |
| **P2** | Reverb shimmer path inside Spring/Shimmer character | **1.4.0** |
| **P2** | Vocoder Phase 2: dedicated APVTS fields, mod-matrix mix/formant | **1.4.0** |

### 5.3 What to cut or never add

| Item | Verdict |
|------|---------|
| `BinauralSpace` in MURMUR | **Cut** — standalone Quasar |
| Convolution reverb slot | **Never** in MURMUR v1.x |
| Multiband compressor / dynamic EQ | **Never** — use EQ + comp |
| Flanger + Phaser + Chorus as 3 slot types | **Defer** — one `Chorus` with mode enum if needed |
| 5th master slot | **No** — 4 is industry-standard (Omnisphere rack = 4; Pigments aux = parallel not more serial) |
| Basic-mode FX params beyond KOINS | **No** — [`ASM_MACRO_KOINS_RESEARCH.md`](ASM_MACRO_KOINS_RESEARCH.md) policy |

### 5.4 Slot architecture: 3 insert + 4 master — still right?

**Yes.** Evidence:

| Product | Insert / pre | Master / post | Total serial |
|---------|--------------|---------------|--------------|
| MURMUR spec | 3 layer insert | 4 master | 7 |
| Pigments | 3+3 insert buses + 3 aux | 9 slots | Comparable |
| Hydrasynth | 1 pre + delay + reverb + 1 post | 4 blocks | Comparable |
| Massive X | 3 per-voice insert | 3 stereo | 6 (+ voice FX differ) |
| Serum 2 | Per-bus unlimited | 2 buses | Flexible but not more than 7 typical |

**Recommended signal flow (v1.2+, Quasar removed):**

```
voices (Layer A [+ B])
  → layer insert FX  I1 → I2 → I3
  → stereo sum (+ masterGain)
  → master FX        M1 → M2 → M3 → M4
  → DAW

(Quasar standalone sits as DAW insert after MURMUR or on headphone bus)
```

**Slot guidance:**

| Slot | Suggested default | Rationale |
|------|-------------------|-----------|
| I1 | Saturation or Chorus | Per-layer tone |
| I2 | Tape Delay | Per-layer rhythm |
| I3 | Bypass or FreqShiftEcho | Design flexibility |
| M1 | Reverb | Space first on master (Hydrasynth/Pigments pattern) |
| M2 | EQ | Carve before comp |
| M3 | Compressor | Glue |
| M4 | Limiter | Safety — always last |

---

## 6. KOINS / macro mapping (SPACE / BLOOM)

Policy: **2 macro KOINS** in Basic/Compact ([`ASM_MACRO_KOINS_RESEARCH.md`](ASM_MACRO_KOINS_RESEARCH.md), [`MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md)). After Quasar extraction, **SPACE must not route to BinauralSpace params**.

### 6.1 BLOOM (Macro1 / KOIN A) — tone & body

| Destination | Amount | When |
|-------------|--------|------|
| `FilterCutoff` | + | Pads, bass clarity |
| `OperatorWavetablePosition` | + | Brightness / harmonics |
| `CompMakeup` or `CompMix` (master M3) | + | Perceived loudness |
| `SaturationDrive` (insert I1) | + (small) | Warmth on bass/leads |
| `EqMidGain` | +/− | Body vs honk |

**Avoid:** Reverb size/decay on BLOOM — that is SPACE territory.

### 6.2 SPACE (Macro2 / KOIN B) — time & ambience (post-Quasar)

| Destination | Amount | Quasar analog (standalone plugin) |
|-------------|--------|-----------------------------------|
| `MasterFxMix` (Reverb slot) | + | Room amount |
| `MasterReverbSize` | + | Room size |
| `MasterReverbDecay` | + (moderate) | Tail length |
| `MasterReverbPreDelay` | + (small) | Distance cue |
| `MasterReverbDiffusion` | + | Smoothness |
| `MasterReverbModDepth` | + | Motion in tail |
| `MasterFxMix` (Tape Delay slot) | + (optional) | Delay wash |
| Tape delay feedback / time | + | Rhythmic space |
| `ChorusDepth` / mix | + (small) | Stereo width substitute |
| `Pan` spread | + | Width without binaural |

**Do not route SPACE to:** `qsr1Angle`, `cntrLevel`, HRTF — those live in **Quasar standalone** presets.

**Patches with `metadata.masterFx: quasar`:** Mission card → *“Spatial: use Quasar plugin — SPACE → Reverb + Delay here.”*

### 6.3 Mod-matrix destinations to prioritize (master FX)

Ship mod destinations for performance macros ([`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md) §B, [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md) §3):

- `MasterReverbMix`, `MasterReverbSize`, `MasterReverbDecay`, `MasterReverbPreDelay`, `MasterReverbDiffusion`, `MasterReverbModDepth`
- `MasterFxMix` (per slot index)
- `CompThreshold`, `CompRatio`, `CompAttack`, `CompRelease`, `CompMakeup`
- Sidechain → existing `ModSource::Sidechain` for comp ducking + vocoder

---

## 7. Cross-links

| Document | Relevance |
|----------|-----------|
| [`GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) | Quasar DSP spec → **standalone plugin**; MURMUR removes `BinauralSpace` slot |
| [`FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md) | Compression deep pass, Reverb character, delay sync roadmap |
| [`FX_BANK.md`](FX_BANK.md) | Current 10+2 algorithms, GATE 10/11 reverb research |
| [`VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md) | Vocoder MVP + Phase 2 — keep as bundled specialty |
| [`ASM_MACRO_KOINS_RESEARCH.md`](ASM_MACRO_KOINS_RESEARCH.md) | 2 macro KOINS policy, SPACE/BLOOM naming |
| [`MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md) | Morph vs macro; INTIMATE ↔ VOID scenes (reverb/delay morph, not Quasar) |
| [`NEUZEIT_QUASAR_RESEARCH.md`](NEUZEIT_QUASAR_RESEARCH.md) | Hardware Quasar → standalone product requirements |
| [`HORIZON2.md`](HORIZON2.md) | Shipped master mod-matrix + Spread KOINS |

---

## Appendix A — MURMUR v1.2+ ship list (checklist)

### Bundled in MURMUR (default)

- [x] EQ (3-band)
- [x] Compressor (+ character deepening)
- [x] Limiter
- [x] Chorus
- [x] Tape Delay (+ tempo sync)
- [x] Reverb (M7 FDN + character)
- [x] Saturation

### Bundled in MURMUR (bank, optional)

- [x] NodeDelay, FreqShiftEcho, FractalEcho
- [x] Vocoder (sidechain)

### Standalone product (not MURMUR)

- [ ] Quasar / BinauralSpace plugin

### Explicitly out of scope

- [ ] Convolution reverb
- [ ] Multiband dynamics
- [ ] Mastering maximizer suite
- [ ] Guitar pedal library

---

## Appendix B — Research sources

| Source | URL / note |
|--------|------------|
| Serum 2 What's New (Xfer) | https://static.xferrecords.com/Serum%202%20What%27s%20New.pdf |
| Vital User Guide (Effects) | https://davidmvogel.com/docs/Vital/UserGuide/Effects |
| Pigments 5 Manual | https://dl.arturia.net/products/pigments/manual/pigments_Manual_5_0_0_EN.pdf |
| Omnisphere 3 FX manual | https://support.spectrasonics.net/manual/Omnisphere3/30/en/topic/fx-page-index |
| Massive X manual (NI) | https://native-instruments.com/ni-tech-manuals/massive-x-manual/en/stereo-effects |
| Diva user guide (u-he) | https://u-he.com/downloads/manuals/plugins/diva/Diva-user-guide.pdf |
| Phase Plant manual (Kilohearts) | https://kilohearts.com/docs/phase_plant |
| Kilohearts Essentials | https://kilohearts.com/products/kilohearts_essentials |
| Hydrasynth specs (ASM) | https://www.asmhydrasynth.com/hydrasynth-specifications.html |
| Waldorf Iridium manual | https://files.kraftmusic.com/media/ownersmanual/Waldorf_Iridium_Keyboard_User_Manual.pdf |
| Arturia Jun-6 V / Jup-8 V manuals | Arturia download center |
