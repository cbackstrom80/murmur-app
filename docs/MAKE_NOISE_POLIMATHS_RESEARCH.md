# Make Noise PoliMATHS → MURMUR KOINS & PLAY Research

**Date:** 2026-08-14  
**Author:** Agent research pass for Curtis  
**Goal:** Identify what Make Noise **PoliMATHS** is (official spelling: capital **MATHS**), document its “one control → many” performance philosophy, and extract actionable ideas for MURMUR KOINS, agentic patch generation, PLAY-only UI, and future hardware (Horizon 3 / Pi CM5).

**Related docs:** `docs/ASM_MACRO_KOINS_RESEARCH.md`, `docs/HORIZON2.md`, `docs/PRODUCT_GAP_PLAN.md`

---

## 1. Product identification

| Property | Value |
|----------|-------|
| **Official name** | **PoliMATHS** (not “Polymaths” — Make Noise preserves the MATHS lineage in caps) |
| **Company** | Make Noise (Eurorack / modular; same family as MATHS, QPAS, Strega, 0-Coast) |
| **Form factor** | **20 HP Eurorack module** — analog/digital hybrid function generator, **not** a standalone synth or software |
| **Release status** | **Shipping** — announced Superbooth 2025; retail availability from **October 2025** |
| **Price** | **$459 MSRP** |
| **Power** | 230 mA @ +12 V, 5 mA @ −12 V |
| **Depth** | 43 mm (incl. power cable) |
| **Firmware** | Browser updater (Chrome); **v1.6.0** recommended (Feb 2026) — adds Binary Counter Span mode, Spread CV range alignment with MultiWAVE, NUSS timing fixes |
| **System context** | Third pillar of **N.U.S.S.** — **N**ew **U**niversal **S**ynthesizer **S**ystem (also marketed as “New Universal Skiff System”) |

### What it is *not*

- **Not** a replacement for the original **MATHS** (still sold separately).
- **Not** a traditional 8-voice polysynth on its own — it is an **8-channel CV/audio event generator** that becomes a voice controller when paired with **MultiWAVE** (osc) + **QXG** (LPG/mix).
- **Not** preset-based — no patch memory, no patch names, no morph between stored states.

### Closest Make Noise analogs (if name were wrong)

| Module | Relationship |
|--------|----------------|
| **MATHS** | Single (dual) rise/fall function generator — DNA of PoliMATHS envelope half |
| **Function** / **0-Coast Slope** | Same slope/envelope lineage |
| **MultiMod** | NUSS “one signal → eight related copies” for modulation (phase/speed flock) |
| **MultiWAVE** | NUSS “one panel → eight wavetable voices” |
| **QXG** | NUSS 4-ch LPG/mixer; two units + PoliMATHS = 8-voice amplitude/VCA chain |
| **QPAS** | Performance filter model in NUSS skiff (stereo multi-peak), not function-gen |

---

## 2. Architecture

PoliMATHS generates **eight independent channel outputs** from **one shared control surface**. Each activated channel produces a **function + optional oscillation**, summed at the channel jack (or submix when enabled).

### Signal path (per channel)

```
Activate / Span / Round / Parallel / Cycle
        │
        ▼
  Channel Activation (1 of 8)
        │
        ├─► Rise–Fall envelope (Curve shaping)
        │         │
        │         └── amplitude envelope for ──┐
        │                                       │
        └─► Variable-shape oscillator ◄─────────┘
              (Rate, Shape, Osc depth, Bias)
                        │
                        ▼
              Strength (bipolar global amplitude)
                        │
                        ▼
              Channel Out 1–8  ──► QXG control header (optional)
```

**Two-layer output:**

1. **Function** — Rise/Fall/Curve envelope (MATHS-family slope generator, ranges tuned for multi-channel Spread).
2. **Oscillation** — Saw ↔ Triangle ↔ Ramp (Shape CV can push to noise); mixed via **Osc** depth; can run LFO-rate or **audio-rate** with **1 V/oct** tracking on Rate CV (attenuverter full CW).

**Oscillation Bias** (long-press Mode): **Unipolar** (AM/tremolo CV) vs **Bipolar** (vibrato/FM/audio source).

### Activation / routing (how eight channels fire)

| Span mode | Activity window | Behavior |
|-----------|-----------------|----------|
| **Channel Index** | White | Span selects channel(s); immediate activation when Activate unpatched; gate-maskable when Activate patched |
| **Round** | Yellow | Each trigger advances N channels (Span sets step); Reset → back to Ch1 |
| **Parallel** | Cyan/Blue | Activate input = clock; per-channel **clock divisions** set by Span |
| **Binary Counter** *(fw 1.6.0)* | Magenta | Activate increments binary counter; channels 1–8 act as **bits** — rhythmic on/off patterns |

Additional routing:

- **Accumulate** — hold activations (orange LEDs) until Accumulate gate; release together.
- **Cycle All** (purple) — each channel self-retriggers at end of Fall (classic MATHS cycle).
- **Follow the Leader** (pink) — end of Fall on Ch N activates Ch N+1 (Ch8 → Ch1).
- **Channel Index Out** — 0.5 V per channel CV; **FLAM Data Delivery** for near-simultaneous multi-channel index to downstream NUSS modules.
- **Submixing** (Cycle+Mode long-press) — unpatched outputs accumulate channels to the left (layered submixes).

### Modulation architecture (the NUSS “macro bus”)

Two complementary systems distribute **one panel/CV value across eight channels differently**:

#### Spread — “Channel Dependent Weighted Modulation Bus”

- Targets **five parameters**: Rise, Fall, Rate, Osc, Strength (gold attenuverter legending).
- **Spread** bipolar control + **Spread CV** sets direction (left channels vs right channels) and depth.
- Per-parameter attenuverters set how much Spread affects each parameter **per channel** (weighted by channel index).
- Manual quote: up to **40 potential targets** (5 params × 8 channels) from one Spread movement.

#### Modulation Dissemination — per-channel sample on activation

- Patch CV into Rise/Fall/Rate/Osc/Strength inputs → parameter **leaves Spread** and uses **Modulation Dissemination**.
- CV value is **captured at the moment that channel Activates** and held for that channel’s event — not a continuous live CV like Curve/Shape.
- Enables: one LFO distributed across eight different Fall times; one mono sequence → eight distinct pitch envelopes on paired voices.

#### Global (live) CV

**Spread, Span, Curve, Shape** respond immediately to panel/CV — suitable for live gestural performance across all active channels.

### NUSS integration

- Rear **header** normals PoliMATHS channel outs → **QXG** control inputs (8-ch VCA/LPG without front-panel cables).
- **MultiWAVE** audio → QXG signal inputs; PoliMATHS → QXG control = **8-voice hybrid polysynth** from ~three modules.
- **MultiMod** provides eight phase/speed-shifted copies of one modulator — Spread/Dissemination on PoliMATHS + flock on MultiMod = deep polyphonic modulation with minimal patching.
- Daisy-chain: Channel Index Out → next PoliMATHS Span CV In → **16 channels**.

---

## 3. Performance UX

PoliMATHS is designed for **live modular performance**, not preset recall.

### Primary performance controls

| Control | Live role |
|---------|-----------|
| **Span** | “Which channel(s)” — strum (Channel Index), step (Round), divide (Parallel), bit pattern (Binary Counter) |
| **Spread** + gold attenuverters | One gesture → different Rise/Fall/Rate/Osc/Strength **per channel** |
| **Rise / Fall / Curve** | Envelope sculpt (Curve CV = global live bend) |
| **Rate / Shape / Osc** | Oscillation timbre & depth (Shape CV = global live morph) |
| **Strength** | Bipolar output polarity/amplitude per channel (via Spread) |
| **Activate / Accumulate / Reset / Cycle gates** | Performance triggering, hold-and-release, sync |
| **Mode / Cycle buttons** | Span mode, cycle mode, oscillation bias, submix toggle |

### Feedback / “UI”

- **Activity windows** per channel: white cursor (selection), green/red (polarity), blue gradient (Spread weight), orange (accumulated).
- **Span / Mode / Cycle** windows: color-coded mode (white/yellow/cyan/magenta; purple/pink cycle; bright white = following external Channel Index).
- No screen, no menu diving — **color + position** encode state (Make Noise “TechniColor” language shared with MultiMod).

### Performance techniques (from manual + demos)

- **Strumming** — Channel Index + Span knob with no Activate patch.
- **Masked sequencing** — Span selects channel; Activate gate stream decides when fires fire.
- **Poly from mono** — Modulation Dissemination + mono CV seq → per-voice parameter identity.
- **Submix performance** — patch one output, layer 1–8 channels on a single bus for additive audio/mod.
- **NUSS lockstep** — Channel Index Out → MultiWAVE / second PoliMATHS for unified voice activation.

### What is absent

- No presets, banks, morph, or A/B.
- No encoder/menu parameter pages.
- **Button settings persist** across power cycle (10 s write delay) — mode, cycle type, oscillation bias, submix — but **not** knob positions or “patches.”

---

## 4. Macro / multi-destination philosophy

PoliMATHS is one of the clearest hardware expressions of **“one control → many parameters”** — comparable in *intent* to ASM Hydrasynth macros and MURMUR KOINS, but implemented as **routing physics** rather than stored modulation matrices.

### Comparison matrix

| Concept | **PoliMATHS Spread** | **PoliMATHS Modulation Dissemination** | **ASM Hydrasynth Macro** | **MURMUR KOINS (target)** |
|---------|----------------------|----------------------------------------|--------------------------|---------------------------|
| Control count | 1 Spread (+ optional CV) | 1 CV per parameter | 1 knob per macro slot | 1–3 featured macro knobs |
| Destinations | Up to 40 (5×8 weighted) | 8 held values per CV input | Up to 8 per macro | 2–4 mod routes per macro |
| Depth model | Per-param attenuverter × channel weight | Sample-on-activate | Signed depth per dest | Signed `amount` in `modRoutes` |
| Live vs staged | Spread CV = live; dissemination = staged per voice | Staged at voice trigger | Live knob sweep | Live APVTS macro 0..1 |
| Naming | None (physical params) | None | 8-char macro name | KOIN label + description |
| Meta-mod | Indirect (Curve/Shape global) | CV can be any source | Macro → mod matrix depth | Planned, not shipped |

### NUSS slogan: “From one comes many”

Shared across **MultiMod**, **MultiWAVE**, and **PoliMATHS**:

- **One** modulator → eight related modulators (MultiMod).
- **One** wavetable panel → eight oscillators (MultiWAVE).
- **One** envelope/osc panel → eight functions (PoliMATHS).

PoliMATHS adds **Spread** as the performance macro layer and **Modulation Dissemination** as **voice-triggered parameter capture** — analogous to **per-note mod matrix sample** or **MPE per-finger offsets**, but in CV land.

### Patch-programming model

“Patches” are **physical + stateful mode choices**, not files:

- Cable norming to QXG/MultiWAVE.
- Span mode + cycle mode + submix on/off (saved).
- Attenuverter “mixes” defining Spread personality.
- External clocks, sequences, and Accumulate wiring.

This is closer to **modular patch sheets** than Hydrasynth preset JSON — relevant for MURMUR **agentic generation** as “expose only what this performance needs” rather than “save 500 parameters.”

---

## 5. Patch storage / curation

| Aspect | PoliMATHS behavior |
|--------|-------------------|
| **Patch memory** | **None** |
| **Named patches** | **None** |
| **Knob recall** | **None** — physical positions only |
| **Saved state** | Button modes only: Span mode, Cycle mode, Osc bias, Submix enable (EEPROM, ~10 s after change) |
| **Minimum exposed controls** | Entire panel always available; **performance curation** is operator-driven (attenuators at noon, init recipe in manual) |
| **Init recipe** | Spread/Shape/attenuators noon; Osc CCW; Strength CW; Channel Index mode — then add gates/CV |

### Curation philosophy (actionable)

Make Noise assumes the player **does not need all 40 Spread targets every moment** — they set **which parameters participate** via gold attenuverters (often near noon until needed). Submix mode reduces **output cable count** while keeping all eight internal activations.

For MURMUR: this mirrors **1–3 KOINS + hide the rest in Advanced** — the patch author (or agent) decides which macro bundles matter for PLAY; the engine still has full depth under the hood.

---

## 6. Relevance to MURMUR

### 6.1 Feature KOINS + standard knobs (Basic/Compact PLAY)

**PoliMATHS lesson:** Separate **voice routing** (Span / activation) from **timbre macro** (Spread on Rise/Fall/Rate/Osc/Strength) from **global live bend** (Curve, Shape).

| MURMUR mapping | Proposal |
|----------------|----------|
| **Span ≈ voice index / arp / round-robin** | Optional future: KOIN or hidden param for “which layer/op is primary” in Compact — *not MVP* |
| **Spread ≈ Macro KOINS** | Each featured macro = one Spread-like bus with 2–4 weighted destinations (already `modRoutes`) |
| **Gold attenuverters ≈ route amounts** | Agent sets relative depths; user gets one knob |
| **Curve/Shape ≈ standard PLAY knobs** | Keep cutoff/reso/LFO as non-KOIN “globals” if patch needs continuous live tweak |
| **Modulation Dissemination ≈ per-note macro offset** | Future: capture macro CV at `noteOn` per voice (MPE-adjacent); would differentiate MURMUR from static macro sweep |

**Concrete KOINS (align with `ASM_MACRO_KOINS_RESEARCH.md`):**

1. **KOIN A (Macro1)** — “Body / envelope” — filter cutoff, op level, env decay (Spread-like: same direction, different depths).
2. **KOIN B (Macro2)** — “Motion / osc” — WT position, warp, LFO depth.
3. **Optional KOIN C (Macro3)** — “Space” — reverb/delay send, stereo width — only when patch is FX-forward.

Use **`macros[i].description`** for Compact hints (Horizon 2 shipped).

### 6.2 Agentic patch generation — expose only what patch needs

PoliMATHS manual **Getting Started** is an **init + minimal wiring** recipe; complexity is opt-in.

| Agent rule | Rationale from PoliMATHS |
|------------|--------------------------|
| **`uiFocus.maxKnobs`: 1–3** | Only put gold attenuverters “on bus” for params that matter |
| **Route macros before naming KOINS** | Dissemination requires routes to exist; empty macro = dash on Hydrasynth |
| **`set_macro_koin` with 2–4 destinations** | Mimics one Spread param with multiple attenuverters |
| **Omit param-kind KOINS** when macros cover the gesture | Submix philosophy: one output cable, many internal channels |
| **Persist mode flags, not knob values** | `.pw8` stores macro *routes* and default macro *values*; PLAY recalls user’s last twist optionally |

MCP enhancement (future): **`set_spread_bundle(macroIndex, destinations[{dest, amount, channelWeight?}])`** — optional channelWeight for future multi-voice Spread emulation.

### 6.3 PLAY-only performance-first UI

| PoliMATHS UX | MURMUR PLAY takeaway |
|--------------|---------------------|
| Color activity windows | KOINS + scope already give feedback; consider **macro activity ring** when macro modulates active voices |
| No preset browser | PLAY focuses on **performance surface**, not patch library management (browser = separate bar) |
| Mode buttons = long-press hidden features | Advanced tab holds “Cycle/Submix equivalent” (mod matrix, ENV shapes) |
| Channel Index strumming | Compact **mission card** + orbiting KOINS = performable without tabbing |
| FLAM Data Delivery | Low priority: if multi-macro triggers needed, batch parameter updates on chord |

**Anti-pattern to avoid:** Showing all eight macros (MacroStrip) in PLAY — PoliMATHS keeps one Spread, not eight Rise knobs.

### 6.4 Horizon 3 / future hardware (Pi CM5)

No CM5 references in repo yet; PoliMATHS still informs **standalone / controller hardware**:

| NUSS / PoliMATHS idea | Pi CM5 / hardware angle |
|-----------------------|-------------------------|
| **8-channel parallelism with one UI** | CM5 has headroom for 8 voice lanes + one performance bus — align with MURMUR voice architecture |
| **Header norming (hidden routing)** | Hardware: pre-wired “voice bus” reduces cable clutter — software: default norming in patch template (osc→filter→VCA) |
| **Channel Index + Spread CV** | Encoder + one **Spread** touch strip → multiple CV destinations (MPE-like) |
| **Firmware updates post-ship** | Binary Counter mode = **feature flags via OTA** on hardware; plugin: schema v3 + migration |
| **USB-C MIDI → MultiWAVE Activate** | CM5 USB host for MIDI/MPE activation without DIN clutter |
| **TechniColor feedback** | LED ring per voice + mode color (Span mode equivalent for arp pattern) |

Horizon 3 differentiation (`PRODUCT_GAP_PLAN.md`): sampler + DESIGN + LAB — PoliMATHS suggests a **LAB performance layer** experimenting with **multi-voice dissemination** before committing to product UI.

---

## 7. References

### Official Make Noise

| Resource | URL |
|----------|-----|
| PoliMATHS product page | https://www.makenoisemusic.com/modules/polimaths/ |
| PoliMATHS manual (PDF) | https://www.analoguehaven.com/make-noise/polimaths/manual.pdf |
| Firmware updater (Chrome) | https://makenoise-manuals.com/firmware/index.html |
| New Universal Skiff System | https://www.makenoisemusic.com/systems/new-universal-skiff-system/ |
| N.U.S.S. Bundle | https://www.makenoisemusic.com/modules/n-u-s-s-bundle/ |
| MultiMod | https://www.makenoisemusic.com/modules/multimod/ |
| MultiWAVE | https://www.makenoisemusic.com/modules/multiwave/ |

### Video

| Title | URL |
|-------|-----|
| Make Noise PoliMATHS and QXG (official launch) | https://www.youtube.com/watch?v=N9DLiMQaOiw |
| PoliMATHS firmware v1.6.0 (Binary Counter) | https://www.youtube.com/watch?v=hbd7vx9QRKU |
| Introducing MultiWAVE (NUSS core completion) | https://www.youtube.com/watch?v=qa1-GdH0M_M |
| Cinematic Laboratory — blind patch day-one | Referenced via Matrixsynth launch post |

### Reviews / community

| Source | URL |
|--------|-----|
| Synth Anatomy — availability / Superbooth context | https://synthanatomy.com/2025/10/make-noise-polimaths-an-8-channel-function-generator-based-on-the-super-popular-maths.html |
| Matrixsynth — PoliMATHS + QXG intro | https://www.matrixsynth.com/2025/10/make-noise-introduces-polimaths-and-qxg.html |
| peaks and nulls — NUSS system sketch (Dec 2025) | https://peaksandnulls.net/index.php/2025/12/23/made-noise-sketch-20-nuss-ahoy/ |
| ModularGrid | https://modulargrid.net/e/make-noise-polimaths |
| SchneidersBuero product copy | https://www.schneiderskeller.co.uk/products/make-noise-polimaths |

### MURMUR internal

| Doc | Path |
|-----|------|
| ASM macro → KOINS research | `docs/ASM_MACRO_KOINS_RESEARCH.md` |
| Horizon 2 shipped scope | `docs/HORIZON2.md` |
| Product gap / Horizon 3 | `docs/PRODUCT_GAP_PLAN.md` |
| Patch format / uiFocus | `docs/PATCH_FORMAT.md` |

---

## Executive summary (for Curtis)

**PoliMATHS is real, shipping, and spelled with capital MATHS.** It is a 20 HP Eurorack **8-channel function/oscillator generator** ($459, Oct 2025) — the modulation/amplitude brain of Make Noise’s **New Universal Synthesizer System**, pairing with MultiWAVE + dual QXG for an 8-voice polysynth without eight knob rows.

Its performance philosophy is **“from one comes many”**: **Spread** is a single performance control that weighted-modulates up to **40 targets** (5 parameters × 8 channels); **Modulation Dissemination** captures CV **per channel at activation** for polyphonic variation from mono sources. There are **no presets** — only mode persistence — so curation means **choosing which parameters are on the bus** (gold attenuverters) and **how many outputs you patch** (submix).

**For MURMUR:** PoliMATHS validates the **1–3 KOINS** direction more strongly than Hydrasynth alone — macros should behave like **Spread buses** (one knob, multiple signed depths), not eight independent params. Agentic generation should **wire `modRoutes` first**, then surface **only routed macros** in `uiFocus`. Future hardware (Horizon 3 / CM5) can borrow **Channel Index + Spread** as a compact multi-voice performance idiom and **OTA mode flags** (Binary Counter-style post-ship features). **Modulation Dissemination** is the standout novel concept — worth a DSP experiment for per-note macro capture on MURMUR voices.
