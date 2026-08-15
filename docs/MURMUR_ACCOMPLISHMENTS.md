# MURMUR Accomplishments — v1.0.9 → v1.1.4

Concise timeline of what shipped on branch `cursor/favorites-unison-stack-daw`. PLAY-only UI (Basic / Compact / Advanced) remains the performance surface; DESIGN mode holds graph/matrix/wavetable editing.

## Factory content (current)

| Bank | Count | Doc |
|------|------:|-----|
| **All factory presets** | **1,079** | [`docs/product/PRESETS.md`](product/PRESETS.md) |
| Interstellar (cinematic) | 100 | [`content/presets/factory/Interstellar/README.md`](../content/presets/factory/Interstellar/README.md) |
| Interstellar **Spatial** (companion QUASAR) | 75 | [`content/presets/factory/Interstellar/Spatial/README.md`](../content/presets/factory/Interstellar/Spatial/README.md) |
| Dissemination showcase | 100 | [`content/presets/factory/Dissemination/README.md`](../content/presets/factory/Dissemination/README.md) |

---

## Timeline

### v1.0.9 — Consolidated Mod Matrix & dual concentric controls

**Release:** [`docs/RELEASE_1.0.9.md`](RELEASE_1.0.9.md)

- **Unified MOD MATRIX** — single routing screen in PLAY **MOD** tab and DESIGN Matrix; deluxe `ObsidianMatrixLookAndFeel` row chrome
- **Dual concentric knobs** — Filter Cutoff/Resonance, WT Bend/Asym on one dial (`ConcentricGlowKnob`)
- **Carried UI polish (1.0.7–1.0.8)** — Live Topology strip, Interstellar HUD badge, wireframe panels, compact teleprompter / mission card
- **900+ factory bank** — 800 core + 100 Interstellar cinematic presets
- **Week 8 engine polish** — tempo-sync LFO targets, golden regression, pluginval soak fixes

### v1.0.10 (AU refresh) — Interstellar Spatial bank

- **75 Spatial Quasar pad presets** with expressive macro KOINS (`content/presets/factory/Interstellar/Spatial/`)
- AU version bump for Logic rescan after Quasar Phase 2 DSP

### v1.1.0 — Quasar Phase 3, Morph KOIN, Sidechain MVP

**Release:** [`docs/RELEASE_1.1.0.md`](RELEASE_1.1.0.md) · **Verify:** [`docs/RELEASE_1.1.0_VERIFY.md`](RELEASE_1.1.0_VERIFY.md)

| Area | Shipped |
|------|---------|
| **Quasar Phase 3** | Dedicated mod destinations (QSR1/2 distance, angle, height, room, delay, CNTR); headphone vs speaker `QuasarOutputMode`; ITD scale + crossfeed; delay freeze at feedback ≥ 0.99; expanded QUASAR FX knobs |
| **Morph KOIN** | `morphPosition` APVTS, `MorphKoinExecutor` runtime lerp across 2–4 keyframes; PLAY morph knob; 20 Spatial morph showcases (INTIMATE ↔ VOID) |
| **Sidechain MVP** | AU `"Sidechain"` input bus, RMS envelope → `ModSource::Sidechain`, performance badge |
| **Horizon 2 baseline** | FILTER FFT scope, KOINS Phase 2 hints, PoliMATHS Spread summaries, **Macro Dissemination MVP** (per-note macro capture Macro1–3) |
| **MCP** | `set_morph_koin`, `set_spread_bundle`, Quasar destination IDs |

**Key docs:** [`docs/GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md), [`docs/MORPH_KOIN_SPEC.md`](MORPH_KOIN_SPEC.md), [`docs/HORIZON2.md`](HORIZON2.md), [`docs/EXT_OSCILLATOR_AU_THEORY.md`](EXT_OSCILLATOR_AU_THEORY.md)

### v1.1.1 — GLOBAL panel + morph rollout

- **`GlobalPanel`** — PLAY Advanced **GLOBAL** tab: CHAIN / QUASAR / OUTPUT sub-tabs ([`docs/HORIZON2.md`](HORIZON2.md))
- **Morph on all 75 Spatial presets** — `scripts/add_morph_koin_spatial_presets.py`
- **Quasar paramOverrides in morph** — angles, heights, room damping in `MorphKoinExecutor.hpp`
- **Sidechain polish** — smoother RMS follower; AU connect hints in badge
- **Meta-mod stub** — [`docs/META_MOD_PLAN.md`](META_MOD_PLAN.md)

### v1.1.2 — Delay tempo sync MVP + mod visual feedback

- **Tape Delay + Quasar post-delay** — host BPM sync with beat subdivisions (1/1 … 1/16, dotted, triplet); FREE ms fallback
- **FX tab + GLOBAL QUASAR** — SYNC / DIV facet rows
- **Mod visual feedback MVP** — GlowKnob ghost pointer for live modulated value; macro activity rings ([`docs/MOD_VISUAL_FEEDBACK.md`](MOD_VISUAL_FEEDBACK.md))
- **Deep-pass planning** — [`docs/FX_DEEP_PASS_PLAN.md`](FX_DEEP_PASS_PLAN.md)

### v1.1.3 — Knob ring visual hierarchy

- **Thicker value / mod route / live-mod ghost** — [`docs/KNOB_RING_SEMANTICS.md`](KNOB_RING_SEMANTICS.md)
- **BASIC PLAY ring legend** — value vs mod route vs live mod dot

### v1.1.4 — Sidechain Vocoder FX + iPad research

- **8-band sidechain vocoder** — `EffectType::Vocoder` (12); AU sidechain as modulator ([`docs/VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md))
- **iPad port research** — [`docs/IPAD_PORT_RESEARCH.md`](IPAD_PORT_RESEARCH.md)

### v1.2.0 — QUASAR standalone extraction (MURMUR)

**Release:** [`docs/RELEASE_1.2.0.md`](RELEASE_1.2.0.md) · **QUASAR:** [`docs/RELEASE_QUASAR_1.0.0.md`](RELEASE_QUASAR_1.0.0.md)

- **Standalone QUASAR plugin** — `pw8_quasar_plugin`, bundle `com.patchwork.quasar`, VST3 + AU + Standalone ([`docs/QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md))
- **MURMUR cleanup** — removed `BinauralSpace` master slot, GLOBAL QUASAR tab, Quasar APVTS, Quasar mod destinations
- **Spatial bank migration** — companion `.quasar` presets in `content/presets/quasar/interstellar/`; M3 Reverb fallback; `"spatial": "use-quasar-plugin"`
- **Build** — `cmake --preset quasar-release`; `scripts/install_quasar_au_local.sh`; `scripts/build_release_quasar_pkg.sh`

---

## Architecture anchors (for audits)

| System | Location |
|--------|----------|
| FX chain (3 insert + 4 master) | [`engine/include/pw8/effects/EffectChain.hpp`](../engine/include/pw8/effects/EffectChain.hpp), [`docs/FX_BANK.md`](FX_BANK.md) |
| Quasar DSP (standalone plugin) | [`engine/include/pw8/effects/BinauralSpace.hpp`](../engine/include/pw8/effects/BinauralSpace.hpp), [`quasar_plugin/`](../quasar_plugin/) |
| PLAY UI | [`docs/product/PLAY_MODE.md`](product/PLAY_MODE.md) |
| Mod matrix | [`docs/MODULATION.md`](MODULATION.md) |

---

## Deferred (documented, not shipped)

- Full `EngineType::External` on operator 0
- Per-voice / per-operator binaural (discrete mono → 3D) — **QUASAR plugin** AU sidechain foundation; see [`docs/QUASAR_STANDALONE_PLUGIN.md`](QUASAR_STANDALONE_PLUGIN.md)
- Meta-mod executor (macro → route depth)
- Dual-filter parallel routing, full PoliMATHS spread channel weights
