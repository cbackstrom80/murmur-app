# MURMUR 1.1.4 — Sidechain Vocoder FX, knob ring semantics, iPad research

Cumulative release **1.1.1 → 1.1.4** on branch `cursor/favorites-unison-stack-daw`, including everything from **1.1.0** (Quasar Phase 3, Morph KOIN, sidechain mod).

## Since 1.1.0 (patch releases)

### v1.1.1 — GLOBAL panel + morph on all Spatial pads

- **PLAY Advanced GLOBAL tab** — CHAIN / QUASAR / OUTPUT sub-panels ([`docs/HORIZON2.md`](HORIZON2.md))
- **Morph KOIN on all 75 Interstellar Spatial presets** — not only the 20 showcases
- **Quasar paramOverrides in morph** — angles, heights, room damping in `MorphKoinExecutor`
- **Sidechain follower polish** — smoother RMS; AU connect hints in badge
- **Meta-mod planning** — [`docs/META_MOD_PLAN.md`](META_MOD_PLAN.md)

### v1.1.2 — Delay tempo sync + mod visual feedback

- **Tape Delay + Quasar post-delay** — host BPM sync (1/1 … 1/16, dotted, triplet); FREE ms fallback
- **FX tab + GLOBAL QUASAR** — SYNC / DIV controls
- **Mod visual feedback MVP** — GlowKnob **ghost pointer** for live modulated value; macro activity rings ([`docs/MOD_VISUAL_FEEDBACK.md`](MOD_VISUAL_FEEDBACK.md))

### v1.1.3 — Knob ring visual hierarchy

- **Thicker value / mod / ghost layers** — clearer inside→out reading on PLAY knobs
- **Ring legend** on BASIC PLAY first KOINS row: `● value arc   ○ mod route   · live mod`
- **Documented semantics** — [`docs/KNOB_RING_SEMANTICS.md`](KNOB_RING_SEMANTICS.md)

### v1.1.4 — 8-band sidechain Vocoder FX

- **`EffectType::Vocoder` (12)** — insert or master FX slot; carrier = synth at chain point, modulator = AU sidechain ([`docs/VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md))
- **8–16 bands**, formant shift, sibilance boost, sidechain gain, mix
- **Shared AU sidechain bus** — mod matrix envelope (v1.1.0) + vocoder bands (v1.1.4)
- **iPad port feasibility** — research doc [`docs/IPAD_PORT_RESEARCH.md`](IPAD_PORT_RESEARCH.md) (no iOS binary in this release)

## Carried from 1.1.0

- **Quasar Phase 3** — dedicated mod destinations, headphone vs speaker, delay freeze, 75 Spatial + 100 Dissemination factory presets
- **Morph KOIN** — runtime morph knob + MCP `set_morph_koin`
- **Sidechain mod (AU)** — Logic sidechain picker → MOD routes

## Tests

- **`ctest --preset dev`** — full dev suite (golden audio regression included)

## Install

Download **`MURMUR-1.1.4-macOS-arm64.pkg`** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases/tag/v1.1.4).

Optional: **`MURMUR-1.1.4-macOS-arm64.dmg`**

After install: quit Logic → Plug-in Manager → Reset & Rescan → confirm **1.1.4**.

## Docs index

| Topic | Doc |
|-------|-----|
| Timeline | [`docs/MURMUR_ACCOMPLISHMENTS.md`](MURMUR_ACCOMPLISHMENTS.md) |
| Quasar roadmap | [`docs/GLOBAL_QUASAR_FX_PLAN.md`](GLOBAL_QUASAR_FX_PLAN.md) |
| Knob rings | [`docs/KNOB_RING_SEMANTICS.md`](KNOB_RING_SEMANTICS.md) |
| Vocoder | [`docs/VOCODER_SIDECHAIN_PLAN.md`](VOCODER_SIDECHAIN_PLAN.md) |
| iPad | [`docs/IPAD_PORT_RESEARCH.md`](IPAD_PORT_RESEARCH.md) |
