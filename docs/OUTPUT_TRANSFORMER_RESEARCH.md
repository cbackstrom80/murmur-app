# Output Transformer Research — Master Compressor Color Stage

**Status:** RESEARCH / PROPOSED (not implemented)  
**Scope:** Post-compression transformer color on the **master-bus Compressor slot only**  
**Reference hardware:** Shadow Hills Mastering Compressor / OptoMax **Trans** switch (Nickel / Iron / Steel)  
**Reference manufacturers:** Jensen, Cinemag, Sowter output transformers  

---

## Why this belongs on the master Compressor

On Shadow Hills hardware the **Trans** control sits in the **output path after dynamics**, not in the detection path. It shapes harmonics, low-end weight, and transient character of the *compressed* signal before makeup gain.

Patchwork Eight's `CompressorProcessor` (`engine/include/pw8/effects/Compressor.hpp`) today is a clean feedforward peak compressor:

- Stereo-linked peak detector → quadratic soft-knee gain computer → attack/release GR smoothing → single combined gain (`GR + makeup`).

There is **no** output-stage color. Adding a transformer macro-model **after GR, before makeup** matches the hardware signal flow and keeps detection transparent.

**Master slot only:** The master bus is where transformer "console output" character belongs. Layer inserts stay surgical; the global Compressor is the one place users expect mix-glue + iron/nickel/steel weight. CPU is paid once per final stereo sample, not per voice.

---

## What transformers actually do (physics → audible)

Audio output transformers are **band-limited, level-dependent, mildly hysteretic** devices. Audible differences come from:

| Mechanism | What you hear |
|-----------|----------------|
| **Core saturation** | Harmonic thickening; increases as level rises; mostly **low–mid band** (published guitar-amp transformer models show strong nonlinearity **below ~30–100 Hz**, but **mix-bus transformers** are driven harder at all frequencies because program material is full-range). |
| **Core material permeability** | **Nickel** (high µ): cleaner, wider bandwidth, less hysteresis. **Iron/steel**: more saturation at lower flux, stronger odd/even harmonic mix, "tighter" or "heavier" lows depending on geometry. |
| **Winding leakage & copper loss** | Gentle HF rolloff, softens extreme transients before saturation. |
| **Magnetizing inductance** | High-pass-ish behavior; bumps/low-lift interaction with load. |
| **Hysteresis** | Frequency-dependent **harmonic** distortion (memory); expensive to simulate; often approximated by asymmetric waveshaping + slow state. |

### Shadow Hills **Trans** (from OptoMax manual, paraphrased)

| Position | Character |
|----------|-----------|
| **Nickel** | Cleanest; least distortion; subtle **ultra-HF** accent. |
| **Iron** | Additional **even-ordered** harmonic content; **musical upper-low** boost (Class-A section in hardware — we approximate with asymmetry + LF shelf). |
| **Steel** | Most colored; **tight low-frequency boost**; strongest "magnetic" transient limiting. |

This is a **3-way core-material switch**, not a brand switch — but the *sonic intent* overlaps how engineers describe Jensen vs Sowter vs Cinemag.

### Manufacturer voicing (informal, for DSP targets)

| Brand | Typical reputation | Useful DSP fingerprint |
|-------|-------------------|------------------------|
| **Jensen** | Transparent, wide, hi-fi balance | Nickel-leaning; minimal odd harmonics; gentle HF air shelf (+0.5 dB @ 10–14 kHz); lowest drive. |
| **Sowter** | British vintage, mid-forward warmth | Iron-leaning; even harmonics; slight **300–800 Hz** presence; softer HF than Jensen. |
| **Cinemag** | American console heritage; variants **H / L / S** (high nickel, 50% nickel, steel) | Maps well to **Nickel / Iron / Steel** respectively on the same topology — use as **fine voicing offsets** on top of core curves. |

**Recommendation:** Ship **one user control** that matches Shadow Hills: **Nickel | Iron | Steel**. Optionally expose **Brand Bias** (Jensen / Cinemag / Sowter) as a second small enum that nudges EQ crossovers and harmonic balance ±10%, not a separate full model — keeps CPU and UX bounded.

---

## Fidelity tiers (what it takes in DSP)

### Tier 1 — Full physics (WDF / SIM / Preisach)

- **Method:** Wave Digital Filters with gyrator–capacitor nonlinear magnetics; scattering iterative solvers; optional Preisach hysteresis (Paiva, Macák, Bernardini et al.).
- **Fidelity:** Can model backward interaction with source impedance, true frequency-dependent saturation.
- **CPU:** Published real-time guitar-amp **output chain including transformer ~7%** of one core @ 96 kHz (2011, unoptimized C++). Multiphysics SIM with multiple nonlinearities: **tens of percent** at 4× OS without DSR optimizations.
- **Verdict for PW8:** **Overkill** for a single master-slot color control next to an 8-line FDN reverb. High implementation and test cost; fragile under automation unless oversampled.

### Tier 2 — Simplified WDF / T-equivalent (linear + one nonlinear inductor)

- **Method:** Linear R–L–C T-network + single nonlinear `f(φ)` branch, solved with 2–5 Newton iterations at OS ×2.
- **CPU:** ~5–15× Tier 3 per sample; manageable for **one** master effect if heavily optimized in C++.
- **Verdict:** Possible **Phase 2** if Tier 3 isn't musically convincing in A/B.

### Tier 3 — Macro-model (recommended v1)

What most **Shadow Hills-style plugins** effectively do: a **post-dynamics shaper + EQ + slow state**, tuned by ear to match nickel/iron/steel captures.

Per stereo sample, after compression GR:

1. **Level-dependent drive** — `drive = base + k · |x| + k_gr · |GR|` so heavy compression pushes more "flux."
2. **Asymmetric waveshaper** — even-heavy for Iron (e.g. `y = x + a·x² + b·x³` with sign-aware coeffs); odd-heavy for Steel; mild tanh for Nickel (reuse `dsp::softSaturate` pattern from `Saturation.hpp`).
3. **2–3 RBJ biquads** (reuse `dsp::Biquad` like `Eq.hpp`):
   - LF shelf (Steel: +1.5 dB @ 90 Hz; Iron: +1 dB @ 180 Hz; Nickel: flat)
   - HF shelf (Nickel: +0.5 dB @ 12 kHz; Iron/Steel: slight rolloff above 16 kHz)
   - Optional peaking (Sowter bias: +0.8 dB @ 450 Hz)
4. **Magnetic slew / transient limiter** — one-pole envelope on `|dx/dt|` scales gain slightly (Steel strongest) — cheap "transformer can't move flux instantly."
5. **Makeup gain** — applied **after** transformer (match OptoMax ordering).

**State:** ~6 biquad states + 1 envelope per channel ≈ **14 floats** — trivial.

**CPU estimate (48 kHz, stereo master, no OS):**

| Block | ops / sample / ch (order of magnitude) |
|-------|----------------------------------------|
| 3 biquads | ~30–45 |
| waveshaper + drive | ~10–15 |
| slew limiter | ~5 |
| **Total** | **~50–80** |

For context, one `CompressorProcessor` sample today is ~20–30 ops. **Full transformer stage ≈ 2–3× compressor cost**, still **≪ 1% of a voice** and negligible vs Reverb FDN. **Feasible.**

At **2× oversampling** on the transformer stage only: still cheap on master bus (~2× Tier 3 cost).

---

## Proposed architecture in PW8

```
Input L/R
   → peak detector + soft-knee GR (existing)
   → apply GR gain (no makeup yet)
   → OutputTransformerStage (NEW, only if compTransformerCore != Bypass)
   → makeup gain
   → dry/wet mix
Output L/R
```

New file: `engine/include/pw8/effects/OutputTransformerStage.hpp`  
Called **only** from `CompressorProcessor` — not shared with Saturation/TapeDelay to avoid scope creep.

### New `EffectSlotParams` fields (compressor-only semantics)

| Field | Type | Purpose |
|-------|------|---------|
| `compTransformerCore` | enum 0–3 | Bypass / Nickel / Iron / Steel |
| `compTransformerBrand` | enum 0–3 | Neutral / Jensen / Cinemag / Sowter (voicing offsets) |
| `compTransformerAmount` | 0–1 | Dry/wet blend of color stage (default 1 when core active) |

Add to `kEffectSlotFieldSpecs` in `PluginState.cpp`, APVTS, serializer — same pattern as existing comp fields.

**Master-only policy (product):**

- DSP: no hard block on layer inserts (automation/host could still set values) — values simply **documented as master-compressor-only**.
- PLAY UI: expose Trans controls **only** when editing a master FX Compressor slot (DESIGN mode later).
- FX wireframe: add nickel/iron/steel schematic to Compressor preview.

### Default presets

| Core | Starting DSP recipe |
|------|---------------------|
| **Nickel** | drive 1.2, tanh k=1.1, HF +0.4 dB @ 12 k, LF flat, slew 0.1 |
| **Iron** | drive 1.8, asym x²=0.08, LF +1 dB @ 200 Hz, HF −0.5 dB @ 14 k, slew 0.2 |
| **Steel** | drive 2.4, tanh k=1.6 + odd, LF +1.8 dB @ 80 Hz, HF −1 dB @ 18 k, slew 0.35 |

Brand enum applies ±15% crossover / harmonic coefficient tweaks (Jensen → less drive; Sowter → mid peak; Cinemag S → steel already close).

---

## Validation plan

1. **Null / bypass** — `compTransformerCore = Bypass` bit-identical to current compressor (regression test).
2. **Harmonic fingerprint** — sine sweep @ −12 dBFS; measure 2nd/3rd harmonic ratio: Iron > Nickel, Steel > Iron at low freqs.
3. **GR interaction** — heavy compression increases THD on transformer stage (drive linked to GR).
4. **Level match** — makeup after color; LUFS within 0.2 dB across cores at same GR.
5. **Optional A/B reference** — capture hardware or commercial SHMC plugin on pink noise + drums (informal tuning, not CI).

Existing tests to extend: `tests/unit/EffectsTests.cpp` compressor cases.

---

## What NOT to do in v1

- Full WDF hysteresis on mix bus
- Per-brand SPICE netlists (Jensen 110K etc.)
- Running transformer on every effect type
- 8× OS on entire master chain

---

## Implementation phases

| Phase | Deliverable | Effort |
|-------|-------------|--------|
| **A** | `OutputTransformerStage.hpp` + unit tests + wire into `CompressorProcessor` | Small |
| **B** | APVTS + serializer + 3 core presets tuned by ear | Small |
| **C** | Master Compressor UI (Trans chips + wireframe) | Medium |
| **D** | Brand bias enum + optional 2× OS on color stage | Small |
| **E** | Tier 2 WDF if Tier 3 A/B fails with mastering engineers | Large |

---

## References (open / published)

- Paiva, Pakarinen, Välimäki, Tikander — *Real-Time Audio Transformer Emulation for Virtual Tube Amplifiers* (EURASIP, 2011) — WDF gyrator–capacitor, low-freq saturation.
- Bernardini et al. — *Toward the Wave Digital Real-Time Emulation of Audio Circuits with Multiple Nonlinearities* (EUSIPCO 2020) — SIM / DSR CPU reductions.
- AES — *Multiphysics Modeling of Audio Circuits With Nonlinear Transformers* (2021) — SIM hierarchy for magnetic cores.
- Shadow Hills OptoMax user manual — **Trans** (Nickel/Iron/Steel) and makeup ordering.
- Cinemag output transformer catalog — H/L/S core variants on same windings.
- Sonarworks — transformer basics (core materials, saturation, hysteresis).

---

## Bottom line

**Yes — a musically useful Cinemag / Sowter / Jensen / nickel–iron–steel palette on the global Compressor is absolutely feasible without meaningful CPU impact**, if implemented as a **Tier 3 macro-model** post-GR (~2–3× current compressor cost, master bus only). Full WDF transformer physics is **not** required for v1 and would be costly relative to benefit.

Recommended UX: **Nickel / Iron / Steel** primary (Shadow Hills parity), with optional **Jensen / Cinemag / Sowter** as subtle brand voicing offsets on the same engine.
