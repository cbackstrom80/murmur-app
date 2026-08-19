# Neuzeit Instruments Quasar → MURMUR Master Bus Research

**Date:** 2026-08-14  
**Author:** Agent research pass for Curtis  
**Branch:** `cursor/favorites-unison-stack-daw`  
**Goal:** Identify what Neuzeit **Quasar** actually is, how it behaves as a master-bus / spatial output processor, and propose how Quasar-*inspired* (not cloned) master FX parameters become addressable to MURMUR’s mod matrix, macro/KOINS, Maths-style CV philosophy, and morph keyframes.

**Related docs:** [`ASM_MACRO_KOINS_RESEARCH.md`](ASM_MACRO_KOINS_RESEARCH.md), [`MAKE_NOISE_POLIMATHS_RESEARCH.md`](MAKE_NOISE_POLIMATHS_RESEARCH.md), [`MUTABLE_INSTRUMENTS_FRAMES_RESEARCH.md`](MUTABLE_INSTRUMENTS_FRAMES_RESEARCH.md), [`MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md), [`HORIZON2.md`](HORIZON2.md), [`FX_BANK.md`](FX_BANK.md)

---

## 1. Product identification

### What Quasar is (verified)

| Property | Value |
|----------|-------|
| **Official name** | **Quasar** (Neuzeit Instruments) — **not** a separate “Quasar DSP” software product |
| **Form factor** | **16 HP Eurorack module** (45 mm deep); Teensy-based digital brain, micro-SD preset storage |
| **Price** | **~$489 USD** / **~£393** (retail; ModularGrid, Sound on Sound, Perfect Circuit, Aug 2026) |
| **Power** | ~185 mA @ +12 V, ~20 mA @ −12 V |
| **I/O** | 2× mono audio inputs (IN1/IN2), stereo line out (2× mono jacks), **dedicated headphone out**, 2× CV inputs (±5 V) |
| **Processing** | 24-bit / 48 kHz I/O, 32-bit internal |
| **Core role** | **Binaural 3D audio mixer** — two independent spatial processors (“Quasar 1” / “Quasar 2”) plus a **CNTR** (center/dry) path |

There is **no** standalone Quasar plugin or desktop app from Neuzeit. Any DAW use is indirect (record modular output or future third-party inspiration).

### Official references

| Source | URL |
|--------|-----|
| Neuzeit product page | https://www.neuzeit-instruments.com/Quasar |
| Manual (PDF) | https://www.analoguehaven.com/neuzeit-instruments/quasar/manual.pdf |
| Firmware 2.0 announcement | https://www.alex4.de/item/neuzeit-instruments-quasar-new-firmware |
| Sound on Sound review | https://www.soundonsound.com/reviews/neuzeit-instruments-quasar |
| Perfect Circuit review | https://www.perfectcircuit.com/signal/neuzeit-quasar-review |
| ModularGrid | https://modulargrid.net/e/neuzeit-instruments-quasar- |

**DivKid:** No dedicated DivKid Quasar video surfaced in this pass; SOS and Perfect Circuit are the strongest long-form reviews. Neuzeit’s own demos and factory presets in the manual remain the primary performance reference.

### Signal path architecture

```
IN1 ──┬──► [HP/LP per route] ──► matrix ──► QSR1 (binaural position + room)
IN2 ──┤                              ├──► QSR2 (binaural position + room)
      └──► [HP/LP per route] ──► matrix ──► CNTR (dry / unprocessed stereo)
                                              │
                    QSR1 + QSR2 + CNTR sum ───┤
                                              ▼
                              [FW ≥2.0: stereo delay post-sum]
                                              │
                                              ▼
                                    stereo out + headphones
```

**Per “Quasar” (QSR1 or QSR2):**

- **Position** in spherical coordinates around the listener’s head:
  - **Height** — above/below
  - **Angle (azimuth)** — 0–360° around head; auto-rotation mode available
  - **Distance** — **20 cm – 10 m** (attenuation + optional Doppler pitch, FW 2.0+)
- **Room reverb** (per QSR, not a standalone reverb module):
  - **Room amount** — 0–100% (distance cue; “how wet is the space around this virtual source”)
  - **Room damping** — HF decay in tail
  - **Room size** — tail length
- **Internal LFO** — speed (Slow/Medium/Fast range up to ~20 Hz), sync to CV clock, many waveforms (sine, ramp, spiral, random, CV-trigger single-shot, etc.), amounts ±100% on Height / Angle / Distance
- **Input matrix** — each input routed independently to QSR1, QSR2, CNTR with per-route volume + optional HPF/LPF

**CNTR path:** Dry-ish center image; bass often high-pass filtered into CNTR while mids/highs orbit QSR1/2 (mono-compatibility pattern in factory presets).

**Firmware 2.0+ delay (post-spatial sum):**

- Types: Tape (pitch on time change), Fade, Reverse (+ L/R flip per repeat)
- Time: 3 ms – 20 s or clock sync (1/16–128× clock)
- Feedback: 0% = single echo; **100% = freeze/loop**
- Band-pass on delay path (HPF base + LPF width, 12 dB/oct analog-style)

**Other performance features:** 128 named presets on SD; Ear Type (Human → Elephant) for HRTF tuning; bypass; binaural-specific psychoacoustic design (head/torso/pinna modeling + room cues).

### What Quasar is *not*

- Not a dedicated algorithmic reverb module (reverb serves **distance perception**)
- Not multi-speaker Atmos/5.1 (headphone-first binaural; speakers get enhanced stereo, not true 3D localization)
- Not a clone target for MURMUR DSP — HRTF + binaural positioning is a large R&D surface (`docs/ROADMAP.md` lists **spatial engine** as PLANNED)

---

## 2. Master bus / global output role

### Quasar in modular vs DAW context

| Context | Typical placement | Role |
|---------|-------------------|------|
| **Eurorack skiff** | Last module before interface / headphone amp | **Master spatial bus**: sum voices → 3D scene → optional delay wash → record |
| **Hybrid** | Send bus return or drum bus insert | Motion on one element while CNTR holds bass/mono core |
| **Headphone-centric production** | Mix bus insert | Anti-“sound inside head” widening without classic Haas-only tricks |

Quasar’s **master-bus personality** is: **dry center anchor (CNTR) + moving wet spatial layers (QSR1/2) + shared room/delay glue** — all driven by a **small set of high-impact params** with **deep CV/LFO routing**.

### What “global master bus output” means for MURMUR

MURMUR (MURMUR engine) renders:

```
voices (Layer A [+ optional Layer B stack])
  → layer insert FX (3 slots)
  → stereo sum
  → master FX chain (4 slots)
  → DAW output
```

Relevant code paths:

| Concern | Location | Shipped state |
|---------|----------|---------------|
| Master gain | `voiceSettings.masterGain` — scales voice bus **before** insert/master FX | APVTS `masterGain`; mod-routable only via future work (today: **not** a `ModDestination`) |
| Layer insert FX | `LayerPatch::insertEffects[3]` | Real DSP (`Engine.cpp` → `layerAInsertChain_`) |
| Master FX | `Patch::masterEffects[4]` | Real DSP (`masterChain_.process`) |
| Reverb algorithm | 8-line FDN + diffuser + early cluster; 15 reverb params per slot | `EffectTypes.hpp`, `FX_BANK.md` GATE 11 |
| Spatial engine | Binaural / 3D positioning | **PLANNED** (`ROADMAP.md` Phase 11) |
| DESIGN FX UI | `DesignFxDetailPanel` | **Planned** in `DESIGN_AND_WARPS_PLAN.md`; **not present** in current tree (grep: no `DesignFxDetailPanel.*` sources). PLAY uses `FxChainStrip` + `FxEffectPlayParams.h` (4 knobs + mix per type) |
| Mod matrix scope | Per-voice only (`Voice::renderSample` → `ModMatrixExecutor`) | **No** master-FX destinations in `ModDestination` enum |

**DESIGN vs PLAY FX:** Product docs (`UI_DIFFERENTIATION_BRIEF.md`) still reference spatial FX in DESIGN (`FilterLfoPanel`, `FxChainStrip`, `DesignFxDetailPanel`). Engine-side **master reverb/EQ/chorus params exist in APVTS** (`masterFx0`…`masterFx3` + ~60 fields per slot in `MurmurProcessor.cpp`); PLAY exposes a **subset** (e.g. reverb: Decay, Size, Pre, Diff). Full M7-style reverb surface is automation/Advanced-only today.

**Quasar analogy on MURMUR master bus today:**

| Quasar concept | MURMUR today | Gap |
|----------------|--------------|-----|
| CNTR dry anchor | Dry signal preserved inside reverb `mix` & parallel paths; no dedicated “center bus” | No explicit dry/center/spatial split |
| QSR1/2 position motion | Pan + chorus width; **no** height/distance | Spatial engine PLANNED |
| Room size/damping/amount | `reverbSizeParam`, `reverbHighRatio`, `reverbDecaySeconds`, `mix` | Params exist; **not mod-matrix targets** |
| CV/LFO → many targets | 8 LFOs + 8 envs + macros → **synth** destinations only | Master FX live outside mod executor |
| Post-sum delay freeze | Fractal/Tape/Node delays on insert or master slots | Different algorithms; freeze via feedback=1 on delay slots |
| One knob → space bundle | Macro KOINS → filter/WT/pan (`ASM_MACRO_KOINS_RESEARCH.md`) | SPACE macro rarely fans to **master** reverb yet |

---

## 3. Parameter surface

### Quasar — key performance parameters

| Group | Parameters | Performance use |
|-------|------------|-----------------|
| **Bus balance** | QSR1 / QSR2 / CNTR level knobs | Dry/wet/spatial mix without repatching |
| **Position (×2)** | Height, Angle, Distance | Core “where is the sound” gestural control |
| **Motion** | LFO speed, LFO amt (H/A/D), Auto rotation, waveforms, CV trigger | Orbits, fly-bys, tremolo, Leslie-like spins |
| **Room (×2)** | Amount, damping, size | Distance cue + ambient wash |
| **Input matrix** | IN→QSR1/2/CNTR volume + HPF/LPF | Frequency-split spatial (bass center, highs orbit) |
| **CV MAP** | 2 CV × 4 targets × ±100% | External performance control (sequencer, MATHS, etc.) |
| **Delay (FW 2.0)** | Type, volume, feedback, time, clock sync, filter | Shared space-time tail after spatial sum |
| **Global** | Ear Type, Distance→Pitch (Doppler), Dual View, Bypass | Tuning and A/B |

### Mapping to MURMUR control paradigms

| Quasar param / behavior | Best MURMUR target | Rationale |
|-------------------------|-------------------|-----------|
| QSR1/2/CNTR balance | **Macro KOIN** (“SPACE”) | One gesture, multiple gains — Hydrasynth/PoliMATHS Spread pattern |
| Room amount + size + damping | **Macro KOIN** + **mod matrix** (LFO→size) | Coherent “hall opens” bundle |
| Height / Angle / Distance motion | **Mod matrix** (LFO→spatial*, when engine exists) | Per-voice vs global motion differs; Quasar uses **global** scene LFO |
| Freeze / 100% delay feedback | **Morph keyframe** or macro step | Discrete performance states (Frames-style) |
| CV MAP multi-target | **`modRoutes`** rows sharing one macro/LFO source | Already how KOINS work; extend destinations |
| Intimate ↔ vast hall | **Morph KOIN** keyframes | `MORPH_KOIN_SPEC.md` already examples `masterEffects[2].mix` |
| Slow orbit vs triggered fly-by | **LFO** vs **Env** (one-shot) routes | Maths-style slope → env retrigger |
| Ear Type / HRTF preset | **Patch metadata** / agent tag | Not a live KOIN — preset authoring |
| Delay time / feedback / type | **Macro** (secondary) or Advanced slot | Less “character” than room+position for pad KOINS |

\*Until spatial engine ships, **pan + reverb mix + size + pre-delay + diffusion** approximate Quasar’s “space opens up” feel on speakers.

### Suggested macro/KOIN bundles (Quasar-inspired)

**SPACE macro (Macro2 / KOIN B)** — fan-out example for agent presets:

| Destination (proposed) | Amount sign | Quasar analog |
|------------------------|-------------|---------------|
| `MasterFxSlot[N].reverbMix` | + | Room amount |
| `MasterFxSlot[N].reverbSizeParam` | + | Room size |
| `MasterFxSlot[N].reverbDecaySeconds` | + (moderate) | Tail length |
| `MasterFxSlot[N].reverbPreDelayMs` | + (small) | Distance pre-delay cue |
| `Pan` or future `SpatialWidth` | + | QSR spread vs CNTR |
| `FilterCutoff` | + (optional) | Air as space opens |

**MOTION macro** — LFO depth on master reverb mod (`reverbModDepth`) + filter cutoff; mirrors Quasar LFO on position/room.

---

## 4. MURMUR integration strategy

### 4.1 Mod matrix — extend `ModDestination`

Today (`ModMatrixTypes.hpp`):

```cpp
enum class ModDestination : std::uint8_t {
    None = 0,
    FilterCutoff, FilterResonance,
    OperatorFilterCutoff, OperatorFilterResonance,
    OperatorLevel, Pan,
    OperatorWavetablePosition, /* … wt params … */
};
```

**Proposal — Horizon 2 (control-rate, patch-level):**

Add master-bus destinations (names illustrative):

| New destination | Param | Scope |
|-----------------|-------|-------|
| `MasterReverbMix` | `masterEffects[slot].mix` when type=Reverb | **Global** |
| `MasterReverbSize` | `reverbSizeParam` | Global |
| `MasterReverbDecay` | `reverbDecaySeconds` | Global |
| `MasterReverbPreDelay` | `reverbPreDelayMs` | Global |
| `MasterReverbDiffusion` | `reverbDiffusion` | Global |
| `MasterReverbModDepth` | `reverbModDepth` | Global |
| `MasterGain` | `voiceSettings.masterGain` | Global |

Implementation sketch:

1. Extend `ModDestination` + serializer/MCP string map (`filter_cutoff` pattern).
2. In `Engine::renderSample` block **before** `masterChain_.process`, run a **patch-level** `ModMatrixExecutor` pass (Global scope only) → write offsets into a `MasterModOffsets` struct applied to a **copy** of `masterEffects` for that sample (or block at 64–256 samples for CPU).
3. Wire `ModRoutingUi`, `MacroSpread.hpp`, MCP `set_macro_koin` destination strings.
4. PLAY: show `SPACE → Reverb Mix, Size, Decay` in preset bar (`spreadSummaryForMacro`).

**Horizon 3:** `SpatialHeight`, `SpatialAngle`, `SpatialDistance`, `SpatialCenterLevel` when spatial engine lands.

### 4.2 Macro / KOINS — SPACE on master bus

Aligns with shipped Horizon 2 KOINS policy (`HORIZON2.md`, `ASM_MACRO_KOINS_RESEARCH.md`):

- **1–3 featured KOINS**; **SPACE** = Macro2 (index 1) on pad/cinematic presets.
- Agent preset tag:

```jsonc
"metadata": {
  "masterFx": "quasar-inspired",
  "performanceHints": ["headphone-friendly spatial wash", "CNTR-style dry low end via low reverb mix on bass patches"]
}
```

- Factory template: master slot 2 or 3 = **Reverb**, mix 0.25–0.45 default; SPACE macro routes as above.
- **Do not** expose 15 reverb params as KOINS — Quasar hides complexity behind QSR menus + CV MAP.

### 4.3 Maths / CV philosophy

Quasar’s **2 CV × 4 targets** maps cleanly to MURMUR’s existing sources:

| Quasar | MURMUR source |
|--------|---------------|
| External CV | **Expression**, **ModWheel**, MP11SE CC → Macro |
| Internal LFO | **Lfo1–8** (Layer/Global scope for “one wobble on whole mix”) |
| Triggered one-shot motion | **Env** one-shot or arp gate → brief mod bump |
| Clock sync | Arp/LFO sync to host tempo |

**Policy:** Treat **master FX mod routes as Global scope** (like Quasar’s scene-wide LFO), distinct from per-voice filter sweeps.

### 4.4 Morph KOIN — keyframe A/B

Use existing `morphKoin` schema (`MORPH_KOIN_SPEC.md`):

| Keyframe | Name | Overrides |
|----------|------|-----------|
| A (0.0) | **INTIMATE** | `masterEffects[2].mix`: 0.12, `reverbSizeParam`: 0.5, `reverbPreDelayMs`: 8, macros: low |
| B (0.45) | **STAGE** | mix 0.35, size 1.0, decay 2.5s |
| C (1.0) | **VOID** | mix 0.72, size 1.8, decay 6s, `reverbDiffusion`: 0.85 |

`uiFocus` example: `{ kind: "morph", label: "EVOLVE" }`, `{ kind: "macro", index: 1, label: "SPACE" }`.

Horizon 3 executor resolves `paramOverrides` paths; Horizon 2 can **author** tags + keyframes for agents/presets even before runtime morph DSP.

### 4.5 Effort & horizon summary

| Work item | Effort | Horizon |
|-----------|--------|---------|
| `ModDestination` master reverb/mix/gain + executor hook in `Engine` | **M** | **2** |
| MCP + `MacroSpread` labels + factory SPACE routes on master reverb | **S** | **2** |
| PLAY Basic: one SPACE KOIN → master reverb fan-out (presets + inference) | **S** | **2** |
| Agent tag `masterFx: quasar-inspired` in preset generator | **S** | **2** |
| `morphKoin` keyframes touching `masterEffects[*]` (metadata + spec) | **S** author / **M** runtime | **2** meta / **3** DSP |
| Binaural spatial engine (QSR-like height/angle/distance) | **L** | **3** |
| Full Quasar delay types on master slot | **M** | **3** (optional; tape/fractal already exist) |

---

## 5. Comparison table

| Dimension | **Neuzeit Quasar** | **Valhalla VintageVerb / Room** | **MURMUR master FX (today)** | **PoliMATHS Spread → master** |
|-----------|-------------------|---------------------------------|------------------------------|-------------------------------|
| **Primary job** | Binaural 3D mixer + room + delay | Algorithmic reverb character | 4-slot bus: sat/chorus/delay/reverb/EQ/comp/limit | One Spread → many **per-voice** params |
| **Spatial model** | H/A/D binaural positions ×2 + CNTR | Stereo width implicit in algo | Stereo FDN reverb; pan at voice layer | 8-channel **voice** timing/level, not reverb bus |
| **Reverb** | Light “room” for distance | Deep, tuned algorithms | 8-line FDN, M7-inspired multiband decay | N/A |
| **Mod routing** | 2 CV × 4 targets; internal LFO | DAW automation only | 8 macros + matrix → **synth** only | Spread attenuverters on Rise/Fall/Rate/Osc/Strength |
| **Performance UX** | Hardware encoders + LED rings | Plugin knobs | 1–3 KOINS + MW/EXP; FX strip abbreviated | No presets; live Spread noon |
| **Master bus** | Intended last stage | Insert on mix bus | `masterEffects[4]` after voice sum | Conceptual parallel: **global** Spread on master params (Horizon 2 proposal) |
| **Headphone story** | Core design target | Good but not binaural | Neutral stereo | N/A |
| **Freeze / infinite tail** | Delay feedback 100% | Infinite decay modes | Reverb decay + delay feedback | Cycle/hold patterns on envelopes |

---

## 6. What NOT to copy

| Do not | Why |
|--------|-----|
| **Clone Quasar binaural DSP / HRTF tables** | Large R&D, headphone-specific, legally/technically distinct from FDN reverb |
| **Clone Quasar UI** (LED rings, QSR menu tree) | Conflicts with Obsidian KOINS / wireframe identity (`UI_DIFFERENTIATION_BRIEF.md`) |
| **Replace M7-style reverb with Quasar room algo** | MURMUR reverb is already researched around Bricasti/FDN principles (`FX_BANK.md`) |
| **Expose all 15 reverb params as Basic KOINS** | Violates 1–3 KOINS policy; Quasar itself hides depth behind CV MAP |
| **Per-voice binaural on 32 voices** | Quasar mixes **two** input buses into a **scene**; MURMUR should keep spatial bus **global** |
| **Market as “Quasar in a plugin”** | Inspiration only — “Quasar-inspired SPACE macro” in metadata is accurate |

**Do copy (ideas):**

- Dry center + wet spatial layers (CNTR + QSR balance **concept** → mix + pan + future spatial)
- One performance control → room amount + size + tail (macro bundle)
- Global LFO on “scene” params (master reverb mod depth/size)
- Frequency-split routing (bass dry / highs wet) via insert EQ + reverb send philosophy
- Agent-oriented preset tags and morph keyframes for intimate ↔ vast

---

## 7. Cross-links & next steps

| Doc | Relevance |
|-----|-----------|
| [`ASM_MACRO_KOINS_RESEARCH.md`](ASM_MACRO_KOINS_RESEARCH.md) | SPACE / BLOOM macro bundles; `set_macro_koin`; 1–3 KOINS policy |
| [`MAKE_NOISE_POLIMATHS_RESEARCH.md`](MAKE_NOISE_POLIMATHS_RESEARCH.md) | Spread fan-out, dissemination, global vs per-voice mod philosophy |
| [`MUTABLE_INSTRUMENTS_FRAMES_RESEARCH.md`](MUTABLE_INSTRUMENTS_FRAMES_RESEARCH.md) | Morph vs macro vs Spread paradigms |
| [`MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md) | Keyframe `paramOverrides` on `masterEffects[2].mix` |
| [`HORIZON2.md`](HORIZON2.md) | Shipped KOINS/MCP; master FX mod matrix = natural Horizon 2 follow-up |
| [`FX_BANK.md`](FX_BANK.md) | Master chain architecture, reverb param semantics |
| [`ROADMAP.md`](ROADMAP.md) | Spatial engine PLANNED; Phase 11 FX partial |

**Recommended immediate actions (Horizon 2):**

1. Add master-reverb `ModDestination` values + Global-scope executor path in `Engine.cpp`.
2. Update Interstellar pad presets: SPACE macro routes to `masterFx*` reverb mix/size/decay.
3. Extend MCP `set_macro_koin` destination enum docs + `MacroSpread` display strings.
4. Document `masterFx: quasar-inspired` in preset generator heuristics.

**Verify in Logic:** Load a pad with master reverb on slot 2; automate Macro2 (SPACE) after step 1 — reverb mix/size should move without opening Advanced FX.

---

## Executive summary (for Curtis)

**Quasar** is a **$489 / 16 HP Eurorack binaural 3D mixer** (two spatial “Quasars” + dry center, per-source room reverb, FW 2.0 post-sum delay)—**not** a standalone plugin. Its master-bus lesson for MURMUR is **scene-level spatial glue**: dry anchor + modulated wet space, with **2 CV × 4 targets**-style routing philosophy.

**MURMUR already has** a 4-slot master FX chain and a deep FDN reverb on APVTS, but **mod matrix / KOINS cannot reach master FX yet** (`ModDestination` stops at pan/operator/filter). `DesignFxDetailPanel` is documented but **not in the tree**; PLAY shows 4 reverb knobs via `FxEffectPlayParams.h`.

**Horizon 2 (M):** Extend mod destinations + Global executor for master reverb mix/size/decay; **SPACE macro KOIN** fans out like Quasar’s room+ distance bundle; agent tag `"masterFx": "quasar-inspired"`. **Horizon 3 (L):** real spatial engine (height/angle/distance)—do **not** clone Quasar DSP or UI.
