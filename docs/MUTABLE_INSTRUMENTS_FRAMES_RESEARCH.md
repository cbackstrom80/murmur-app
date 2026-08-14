# Mutable Instruments Frames → MURMUR KOINS & Morph Research

**Date:** 2026-08-14  
**Author:** Agent research pass for Curtis  
**Goal:** Document what **Mutable Instruments Frames** is (Émilie Gillet / MI, discontinued but influential), how its **keyframe morph** philosophy differs from PoliMATHS **Spread** and ASM **macro bundles**, and extract actionable ideas for MURMUR KOINS, agentic patch generation, and PLAY-only UI.

**Related docs:** `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md`, `docs/ASM_MACRO_KOINS_RESEARCH.md`, `docs/MORPH_KOIN_SPEC.md`, `docs/HORIZON2.md`, `docs/PATCH_FORMAT.md`

---

## 1. Product — what Frames is

| Property | Value |
|----------|-------|
| **Official name** | **Frames** (Mutable Instruments) |
| **Designer** | Émilie Gillet (Mutable Instruments) |
| **Form factor** | **18 HP Eurorack module** — hybrid **4× digitally controlled analog VCA** + keyframe animator |
| **Release era** | ~2014; **discontinued** with MI wind-down (company ceased new module production; firmware/hardware remain open source) |
| **Power** | 90 mA @ +12 V, 30 mA @ −12 V |
| **Price (historical)** | ~£235 / ~$260 retail |
| **Open source** | **Yes** — MIT firmware, CC-BY-SA 3.0 hardware (`pichenettes/eurorack/frames`) |

### What it does

Frames is **not primarily a mixer** — it is a **keyframer** wrapped around four DC-coupled VCAs. Each channel has independent input/output; flexible normalling turns the same hardware into:

- **4-channel mixer / crossfader** (blend audio sources at the MIX bus)
- **Quad panner / dispatcher** (one source → four destinations with animated levels)
- **4-channel programmable CV source** (+10 V offset switch → constant voltage through VCAs)
- **Multi-stage envelope / sequencer** (when FRAME CV is driven by gates or slopes)
- **Hidden quadrature wavetable LFO** (alternate mode — MI “easter egg”)

The signature feature: store up to **64 keyframes** along a timeline; the big **FRAME** knob (or FRAME CV) **interpolates** between adjacent keyframes, animating all four channel gains simultaneously.

### What it is *not*

- **Not** a synthesizer voice or preset synth — no oscillators, filters, or patch names.
- **Not** a live “one knob → many params” macro matrix (that is Hydrasynth / PoliMATHS Spread territory).
- **Not** a stored-voltage sequencer in stock mode (though sequencer sub-mode and Parasite firmware add stepping).
- **Not** polyphonic — one morph position, four parallel levels/CVs.

### Closest MI / modular analogs

| Module | Relationship |
|--------|--------------|
| **Stages** | Later MI module; segment-based CV sequencing — community often compares to Frames’ stored-voltage concept |
| **Marbles** | Random/sequenced CV; different paradigm (generative vs authored keyframes) |
| **Braids / Plaits** | Frequently patched *into* Frames inputs; Frames animates mix or sends TIMBRE CV back |
| **Morphader** (Intellijel) | CV crossfader between two paths — simpler 2-way morph, no keyframe timeline |
| **Make Noise PoliMATHS Spread** | One live control → many weighted targets — **no stored states**, opposite of keyframe morph |

---

## 2. Core UX — keyframes, BIG KNOB, per-channel control

### Panel layout (mental model)

```
┌─────────────────────────────────────────┐
│  [1]  [2]  [3]  [4]   ← channel gain knobs (A/B/C/D)
│   ●    ●    ●    ●    ← level LEDs
│                                         │
│         ╭───────╮                       │
│         │ FRAME │  ← BIG KNOB (timeline) │
│         ╰───────╯   RGB color = position │
│                                         │
│  [ADD]  [DEL]     MODULATION attenuverter
│                                         │
│  ALL in ──► 1 2 3 4 outs ──► MIX out   │
│  FRAME CV in          FR.STEP trigger out│
└─────────────────────────────────────────┘
```

### Keyframe workflow

1. Turn **FRAME** knob to a position on the timeline (7 o'clock = start, 5 o'clock = end).
2. Press **ADD** — creates a keyframe at that position (keyframe LED **G** lights; FRAME knob gets a unique color).
3. Adjust knobs **1–4** — store channel gains **only while keyframe LED is on**.
4. Repeat at other FRAME positions — up to **64 keyframes**.
5. Rotate **FRAME** — module **interpolates** gains between neighboring keyframes; RGB knob color blends between keyframe hues.

**DEL** removes the keyframe at the current FRAME position. Between keyframes (no keyframe at play-head), knobs 1–4 are **read-only** — values are computed by interpolation.

### Per-channel modulation options

| Feature | How | Effect |
|---------|-----|--------|
| **Interpolation curve** | Hold ADD 1 s → turn channel knob | Step, linear, accelerating, decelerating, smooth (raised cosine), bouncing — **per channel** |
| **Response curve** | Hold DEL 1 s → turn channel knob | Linear → logarithmic gain law — **per channel** |
| **FRAME CV** | Patch LFO/envelope/sequencer → FRAME in | Auto-play animation; MODULATION attenuverter sets direction/depth |
| **FR.STEP out** | — | 1 ms +5 V trigger when play-head **crosses** a keyframe — sync external events |
| **Sequencer sub-mode** | At existing keyframe, press ADD **5×** | FRAME knob disabled; CV pulses step keyframes **without interpolation** |
| **+10 V offset** | Rear/switch | Turns unused inputs into CV sources (VCA × constant voltage) |

### Visual feedback (hardware UX)

- **FRAME knob RGB** — each keyframe gets a color; intermediate positions blend hues (timeline as color wheel).
- **Channel LEDs 1–4** — show current interpolated gain.
- **Keyframe LED G** — lit only when play-head sits on a stored keyframe (editable).

This LED-ring + color-morph feedback is iconic Frames UX — influential but **not** directly portable to MURMUR’s Obsidian skin (see §7).

---

## 3. Morphing philosophy — keyframes vs Spread vs macros

Three distinct “one control → many outcomes” paradigms:

### Frames: **interpolate between saved states**

```
Keyframe A          Keyframe B          Keyframe C
  ch1=100%              ch1=0%              ch1=50%
  ch2=0%                ch2=100%            ch2=50%
       \                   |                   /
        \                  |                  /
         ═══════ FRAME knob / CV ═══════════
                    (single play-head)
```

- **Authoring model:** Record discrete **snapshots** (4 gains each) at timeline positions; performance = **scrub/play-head** through the timeline.
- **Live editing:** Only at keyframe positions; between them, all four channels are **derived**.
- **Time dimension:** Explicit — keyframes have **positions** on a 1D timeline (not just A/B/C/D labels, though 2–4 keyframes is the common musical case).
- **Best for:** Crossfades, evolving mixes, CV automation curves, quad pan trajectories, multi-stage envelopes.

### PoliMATHS Spread: **fan out one live value**

(See `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md`.)

- One **Spread** control + per-parameter attenuverters → **different depths per channel**, all computed **live** from the current Spread position.
- **No stored states** — turning Spread left/right immediately reweights all eight channels.
- **Best for:** Polyphonic variation, strummed envelopes, live gestural “tilt” across voices.

### ASM Hydrasynth macros: **sweep destinations from current baseline**

(See `docs/ASM_MACRO_KOINS_RESEARCH.md`.)

- One macro knob → up to 8 destinations with signed depths; **live sweep** from stored patch baseline.
- **No timeline** — macro value is a single 0..1 (or bipolar) axis.
- **Best for:** Performance timbre/space bundles on a preset.

### MURMUR KOINS (today)

- **Macro KOINS** = Hydrasynth-style bundles via `modRoutes` (live macro sweep).
- **Dissemination** (`voiceSettings.macroDissemination`) = **per-note macro snapshot** at `noteOn` — closer to “freeze this moment” than Frames morph, but **one axis per macro**, not interpolation between named patch states.
- **`layerMorph`** (schema field, PLANNED DSP) = equal-power crossfade between **layer A and layer B** — 2-state morph, not N keyframes.

**Frames’ unique lesson for MURMUR:** Separate **(a) morph position** (where am I on the timeline?) from **(b) keyframe contents** (what are the four/channel bundles at each stop?) from **(c) live macro spread** (PoliMATHS/Hydrasynth continuous fan-out).

---

## 4. Comparison table

| Dimension | **Frames** | **PoliMATHS Spread** | **ASM Hydrasynth Macro** | **MURMUR KOINS (shipped + target)** |
|-----------|------------|----------------------|--------------------------|-------------------------------------|
| **Primary control** | FRAME knob / CV (timeline) | Spread knob + CV | Macro 1–8 knobs | Macro1–3 KOINS (1–3 featured) |
| **Destinations** | 4 channel gains (or 4 CV outs) | Up to 40 (5 params × 8 ch) | Up to 8 per macro | 2–4 mod routes per macro |
| **State model** | Up to 64 **stored keyframes** | **No storage** — live only | Patch stores routes + macro default | `.pw8` stores routes + macro values |
| **Morph type** | **Interpolate** between snapshots | **Weighted fan-out** from one value | **Live sweep** of destination depths | Live sweep; dissemination = note-on freeze |
| **Timeline** | Yes — keyframe positions on FRAME axis | No | No | No (except planned `layerMorph` 2-state) |
| **Per-dest curves** | 6 easing × 4 channels + log response | Spread attenuverters (linear weight) | Linear depth (no curve menu) | Linear `amount` in mod matrix |
| **CV automation** | FRAME CV = core use case | Spread CV = live | MIDI CC / mod matrix | DAW automation on macro APVTS |
| **Preset recall** | Save config (ADD 5 s); keyframes in EEPROM | Modes only (no knob recall) | Full patch | Full `.pw8` + host state |
| **Performance UX** | One big morph + 4 edit knobs | One Spread + Span routing | 1–8 Home macros | 1–3 decked KOINS + MW/EXP |
| **Agentic fit** | Keyframe **names/roles** as patch metadata | Spread bundle + dissemination | `set_macro_koin` + routes | Shipped: `set_macro_koin`, spread hints |

---

## 5. Patch / memory model — what gets stored vs live

| Data | Stored (EEPROM) | Live only |
|------|-----------------|-----------|
| **Keyframe positions** | Yes — up to 64 on FRAME timeline | Play-head position follows knob/CV |
| **Per-keyframe gains (ch 1–4)** | Yes | Interpolated values between keyframes |
| **Per-channel easing curve** | Yes | Preview on FRAME knob while editing |
| **Per-channel response curve** | Yes | — |
| **Timeline / sequencer mode flag** | Yes (in saved config) | — |
| **+10 V offset switch** | Physical | — |
| **Input/output patching** | Physical cables | — |
| **Current FRAME position at power-off** | Restored with saved config | — |

**Save/load:**

- **ADD 5 s** — persist entire module config (keyframes + curves + mode).
- **DEL 5 s** — clear all keyframes (reset animation).
- Power-on plays a short RGB “hello” animation.

**Contrast with MURMUR `.pw8`:**

| Frames concept | MURMUR equivalent today | Gap |
|----------------|-------------------------|-----|
| Keyframe gain vector (4 floats) | `macros[i].value` + `modRoutes` amounts | Macros are 1D, not 4-tuples at positions |
| FRAME position | Macro APVTS value / automation | No timeline index |
| Interpolation between keyframes | — | Not implemented |
| Per-keyframe easing | — | Not implemented |
| FR.STEP sync | — | Could map to bar/phrase triggers (Horizon 3) |
| Save config | Full preset JSON | Already richer than Frames |

**Contrast with PoliMATHS:** PoliMATHS saves **mode flags** only; Frames saves **full animation data** — closer to MURMUR preset files, but limited to four numbers per snapshot.

---

## 6. Relevance to MURMUR — actionable ideas

**Context (already shipped):** 1–3 feature macro KOINS, `modRoutes` spread summaries, Modulation Dissemination MVP, `set_macro_koin` MCP tool — see `docs/HORIZON2.md` and `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md`.

### 6.1 Could a KOIN be a **morph between 2–4 patch snapshots**?

**Concept:** One performance KOIN = **FRAME knob** — crossfade between 2–4 **named snapshot states** of selected parameters (not full `.pw8` clone — curated subset).

| Snapshot slot | Example content |
|---------------|-----------------|
| **A “Dry”** | Filter closed, low WT position, short env |
| **B “Open”** | Filter open, high WT position, long env |
| **C “Broken”** | Bitcrush up, warp max, wide pan |
| **D “Space”** | High reverb send, slow LFO |

**Implementation sketch:**

```jsonc
"morphKoin": {
  "label": "EVOLVE",
  "position": 0.35,
  "keyframes": [
    { "name": "TIGHT", "weights": { "filterCutoffHz": 800, "macro1": 0.2 } },
    { "name": "WIDE",  "weights": { "filterCutoffHz": 12000, "macro1": 0.8 } }
  ],
  "curve": "smooth",
  "exposeAsKoin": true
}
```

At runtime: `param = lerp(keyframe[i], keyframe[i+1], localT)` with optional easing — **orthogonal** to `modRoutes` (which add **delta** from macro value, not replace base state).

**Relation to `layerMorph`:** `layerMorph` is **2-layer signal-path crossfade** (PLANNED DSP). Morph KOIN is **multi-parameter performance crossfade** within one layer — complementary; could drive `layerMorph` as one destination.

| Effort | Horizon |
|--------|---------|
| **L** (schema, executor, UI, agent tools, preset migration) | **Horizon 3** |

### 6.2 Keyframe presets for agentic patches — expose morph position as 1 KOIN?

**Agent workflow:**

1. Agent authors 2–4 **keyframes** as semantic patch moments (`TIGHT`, `BLOOM`, `BREAK`, `VOID`).
2. Agent sets default **morph position** (e.g. 0.2 = mostly TIGHT).
3. **`uiFocus`** exposes **one morph KOIN** (replaces or supplements a macro KOIN slot).
4. **`macros[i].description`** / mission card: `"EVOLVE: TIGHT ↔ BLOOM — filter, WT, space"`.

**MCP tools (future):**

- `set_morph_koin(patch_id, keyframes[], default_position, curve)`
- `add_morph_keyframe(patch_id, name, param_snapshot)`

**Synergy with dissemination:** Morph position could be **captured at note-on** (like macro dissemination) so held chords stay on the morph point they were played — hybrid Frames + PoliMATHS.

| Effort | Horizon |
|--------|---------|
| **M** (agent schema + inference without full executor) | **Horizon 2** (metadata-only) / **Horizon 3** (DSP) |

### 6.3 Basic / Compact PLAY: one morph knob + 2 feature macros?

**Proposed PLAY layout (Frames-inspired):**

| Control | Role | Analog |
|---------|------|--------|
| **KOIN MORPH** | Timeline between 2–4 snapshots | FRAME knob |
| **KOIN A (Macro1)** | Live spread — timbre body | Hydrasynth macro / PoliMATHS Spread |
| **KOIN B (Macro2)** | Live spread — motion/space | Same |
| MW / EXP | Expressive MIDI (unchanged) | — |

**Rationale:** Frames separates **morph** (between saved worlds) from **channel gains** (within a world). MURMUR can separate **morph KOIN** (scene crossfade) from **macro KOINS** (continuous multi-dest sweep within a scene). Compact teleprompter: morph at scope hub center, Macro1/2 in orbit (3 KOINS max — fits `uiFocus.maxKnobs: 3`).

| Effort | Horizon |
|--------|---------|
| **M** (UI layout + policy docs) | **Horizon 2** (if morph is macro-only) / **Horizon 3** (true snapshot morph) |

### 6.4 Relation to existing macro / modRoutes vs snapshot morph

| Mechanism | What moves | When | Frames analog |
|-----------|------------|------|---------------|
| **`modRoutes` + macro value** | Destination deltas from baseline | Live; optionally frozen per note (dissemination) | Like turning ch 1–4 knobs **at one keyframe** |
| **Dissemination** | Macro value at `noteOn` | Per voice, held | Sample one timeline point per voice |
| **Spread channel weights** (H2 backlog) | Same macro → different depth per voice | Live | Per-channel attenuverter |
| **Snapshot morph KOIN** | Interpolated **base values** across keyframes | Live; optionally frozen | **FRAME knob** between keyframes |
| **`layerMorph`** | Layer A vs B signal path | Live (when wired) | 2-input crossfader only |

**Design rule:** Do **not** overload macro KOINS to mean both Spread **and** keyframe morph — different mental models. If both ship, use distinct KOIN kinds: `kind: macro` vs `kind: morph`.

**Low-effort bridge (Horizon 2):** Author **two macro snapshots** as separate presets in a bank; morph KOIN = crossfade Macro1 default values between preset A/B loaded as keyframe data — prototype without new DSP.

| Effort | Horizon |
|--------|---------|
| **S** (document pattern in factory bank) | **Horizon 2** |
| **M–L** (first-class morph executor) | **Horizon 3** |

### 6.5 Additional Frames ideas worth mining

| Frames feature | MURMUR idea | Effort | Horizon |
|----------------|-------------|--------|---------|
| **Per-channel easing** | Per-route curve on macro/morph destinations | M | 3 |
| **FR.STEP trigger** | Phrase/bar pulse for arp or preset advance | M | 3 |
| **Sequencer mode (no interp)** | Stepped macro scenes (button/KOIN toggle) | S | 2 |
| **Quad CV mode (+10 V)** | Macro generates 4 CVs from one morph — agent “quad bundle” | L | 3 |
| **Autoplay (Parasite)** | LFO on morph position for pads | S | 2 |
| **Keyframe wraparound (Parasite)** | Morph past last keyframe → blend last→first (loop) | S | 3 |

---

## 7. What NOT to copy

| Frames constraint | Why skip for MURMUR | Instead |
|-------------------|---------------------|---------|
| **4-channel mixer scope** | MURMUR is a synth engine, not a quad VCA | Use morph on **musical params** (filter, WT, FX), not audio bus gains |
| **Hardware RGB FRAME ring** | Obsidian UI uses decked knobs + scope hub, not MI color wheel | Optional **morph hue on scope hub** or mission card gradient — subtle, not literal |
| **64 keyframes** | Overkill for PLAY; agent/user curation | Cap at **2–4 keyframes** per morph KOIN |
| **Edit-only-at-keyframe** | Software can allow live tweak everywhere | Advanced tab always editable; PLAY shows morph + macros only |
| **No labels / no patch names** | Software affordance | Keyframe names in mission card + `macros[i].description` |
| **Physical normalling** | Virtual graph already has routing | Default patch templates (PoliMATHS submix lesson) |
| **Single timeline only** | Multi-dimensional performance | Macros handle live spread; morph handles scene change |
| **Stepped sequencer sub-mode (5× ADD)** | Obscure UX | Explicit **scene buttons** or macro button modes (ASM Toggle) |

---

## 8. Effort / Horizon summary

| Idea | Effort | Horizon | Notes |
|------|--------|---------|-------|
| Document Frames vs Spread vs macro in agent prompts | **S** | **2** | This doc + cross-links |
| Factory **dual-preset morph** demo (two `.pw8` scenes, manual A/B) | **S** | **2** | No code — validation only |
| **`kind: morph` in uiFocus** (metadata + mission card) | **M** | **2** | UI label before DSP |
| **Morph KOIN executor** (2–4 keyframes, lerp + easing) | **L** | **3** | Schema v3, engine, MCP |
| **Morph + dissemination** (freeze morph at note-on) | **M** | **3** | Extends `voiceSettings` |
| **One morph + 2 macro PLAY layout** | **M** | **2–3** | Policy + Compact orbit |
| **Per-route easing curves** | **M** | **3** | ASM/Frames parity |
| **FR.STEP / phrase sync** | **M** | **3** | Arp/preset advance |
| **layerMorph DSP wiring** | **L** | **3** | Already PLANNED — separate from morph KOIN |

### Priority backlog (Curtis)

| Priority | Item | Effort | Horizon |
|----------|------|--------|---------|
| **P1** | Keep **macro KOINS** as Spread analog; don’t conflate with morph | S | 2 |
| **P2** | Prototype **2-keyframe morph** as paired factory presets + mission copy | S | 2 |
| **P2** | Spec **`morphKoin` schema** + `set_morph_koin` MCP (metadata first) | M | 2 | ✅ See `docs/MORPH_KOIN_SPEC.md` |
| **P3** | **Morph executor** + Compact 3-KOIN layout (morph + 2 macros) | L | 3 |
| **P3** | Morph position + **dissemination freeze** | M | 3 |
| **P4** | Per-route easing, FR.STEP sync | M | 3 |

---

## 9. References

### Official Mutable Instruments

| Resource | URL |
|----------|-----|
| Frames manual (ReadTheDocs mirror) | https://pichenette.readthedocs.io/en/latest/modules/frames/manual/ |
| Frames manual (GitHub pages mirror) | https://pichenettes.github.io/mutable-instruments-documentation/modules/frames/manual/ |
| Frames quickstart PDF | https://pichenettes.github.io/mutable-instruments-documentation/modules/frames/downloads/frames_quickstart.pdf |
| Open source / firmware hacking | https://pichenettes.github.io/mutable-instruments-documentation/modules/frames/open_source/ |
| Eurorack repo — `frames/` | https://github.com/pichenettes/eurorack/tree/master/frames |
| Keyframer source | https://github.com/pichenettes/eurorack/blob/master/frames/keyframer.h |
| ModularGrid | https://modulargrid.net/e/mutable-instruments-frames |

### Video & community

| Title | URL |
|-------|-----|
| **DivKid — Frames Overview and Basics Tutorial** | https://www.youtube.com/watch?v=LfdoG8kKWIc |
| DivKid — Modular Jam #26 (Frames in performance) | https://divkidvideo.com/videos/modular-jam-26-lonely-mutant/ |
| **Parasite alt firmware** (Euclidean seq, autoplay, presets) | http://mqtthiqs.github.io/parasites/frames.html |
| MOD WIGGLER — Frames discussion | https://modwiggler.com/forum/viewtopic.php?t=95360 |

### Retail / archival

| Source | URL |
|--------|-----|
| Postmodular (archival listing) | https://postmodular.co.uk/modules/frames/ |
| Big City Music (archival copy) | https://www.bigcitymusic.com/products/frames-mixer-keyframer |

### MURMUR internal

| Doc | Path |
|-----|------|
| PoliMATHS → KOINS research | `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md` |
| ASM macro → KOINS research | `docs/ASM_MACRO_KOINS_RESEARCH.md` |
| Horizon 2 shipped scope | `docs/HORIZON2.md` |
| Patch format / uiFocus | `docs/PATCH_FORMAT.md` |
| **Morph KOIN spec (Horizon 2)** | `docs/MORPH_KOIN_SPEC.md` |
| Prior art (Morphader / layerMorph) | `docs/PRIOR_ART.md` |
| Dissemination factory bank | `content/presets/factory/Dissemination/` |

---

## Executive summary (for Curtis)

**Mutable Instruments Frames** (Émilie Gillet, ~2014, discontinued) is an **18 HP keyframer**: four DC-coupled VCAs whose gains are stored as up to **64 keyframes** on a timeline and **interpolated** by one **FRAME** knob or CV. It is influential for **morph-between-snapshots** performance — not for synthesis per se, but for animation-style control of mixes, pans, and CV trajectories. Firmware is **open source** (MIT); Parasite alt firmware adds autoplay, Euclidean sequencing, and preset slots.

**vs PoliMATHS Spread:** Spread **fans one live value** across many weighted targets with **no stored states**. Frames **interpolates between saved 4-tuples** along a timeline. PoliMATHS = live poly gesture; Frames = authored scene crossfade.

**vs ASM / MURMUR KOINS:** Hydrasynth macros and MURMUR macro KOINS **sweep multiple destinations live** from one axis (`modRoutes`). Dissemination **freezes macro value at note-on** — closer to sampling one timeline point, but **not** blending between named keyframes. Planned **`layerMorph`** is only a 2-layer signal crossfade.

**Actionable for MURMUR:** Consider a distinct **`morph` KOIN** (2–4 named snapshot states, one timeline knob) **alongside** 1–2 **Spread-style macro KOINS** — Basic/Compact = morph + BLOOM + SPACE. Start **Horizon 2** with schema/metadata + factory A/B demos; **Horizon 3** for morph executor + easing + dissemination freeze. **Do not copy** the 4-channel mixer scope or literal RGB FRAME ring UX.

**Next P2:** ~~Spec `morphKoin` + agent MCP metadata~~ Done — `docs/MORPH_KOIN_SPEC.md` + `set_morph_koin`; prototype with paired factory presets before DSP.
