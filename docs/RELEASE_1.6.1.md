# MURMUR 1.6.1 — Real-Time Safety & Performance

Customer-ready build for **Apple Silicon Macs** (M1/M2/M3/M4/M2 Ultra), **macOS 13+**, **Logic Pro**.

## Install (Ben)

1. Download **`MURMUR-1.6.1-macOS-arm64-full.dmg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases)
2. Open the DMG → double-click **`Install MURMUR.pkg`**
3. If Gatekeeper blocks it: **right-click → Open → Open**
4. **Quit Logic completely**, reopen
5. **Settings → Plug-in Manager → Reset & Rescan Selection**
6. Software Instrument track → **AU Instruments → Murmur → MURMUR**
7. Click the **preset name** in the header → **Preset Explorer** → load a factory patch

Optional: drag **MURMUR.app** from the DMG to **Applications** to test standalone (no Logic required).

## What's included

| Item | Path after install |
|------|-------------------|
| Audio Unit (Logic) | `~/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| Standalone app | `/Applications/MURMUR.app` (full installer only) |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/MURMUR.vst3` (full installer only) |
| Factory presets (1,129) | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Design FX presets | `~/Library/Application Support/MURMUR/design-fx/` |

## What's new since 1.6.0

A behind-the-scenes real-time-safety and performance pass — no new UI, no new engines. Everything below was verified end-to-end (real before/after audio diffs, not just "tests pass").

- **Live parameter updates are now genuinely event-driven.** Previously, every audio block rebuilt every operator/LFO/envelope/filter's live parameters regardless of whether anything actually changed — a real (if usually inaudible) CPU cost, worse the more voices are sounding. Automation and knob changes now only trigger work for the parameter groups that actually changed.
- **Lower average CPU load**, most noticeable on patches using master-bus effects, active modulation, or granular textures — internal benchmarks measured a real ~17% reduction on the core per-voice render path and ~2-5% on full-patch rendering.
- **Fixed a real pitch-independent tuning inconsistency**: the Classic engine's Triangle waveform and the Phase/Shape engine's Wavefold amount both had a subtle character that shifted slightly with project sample rate (44.1kHz vs. 48kHz vs. 96kHz) — the same patch could sound very slightly different depending on your session's sample rate. Both now render consistently across sample rates.
- **Granular engine's grain envelope** now uses a precomputed lookup table instead of a live trig calculation per grain per sample — measurably cheaper with no audible difference (verified to a fraction of a millionth of full scale).

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Logic says "incompatible" | Delete old MURMUR.component, reinstall 1.6.1 pkg, rescan |
| Blank plugin window | Reinstall pkg (content missing); try Standalone app first |
| A saved project sounds slightly different after updating | Only affects Triangle/Wavefold-heavy patches rendered at a non-48kHz sample rate — the new render is the *more* correct one; if you need the exact previous character, contact support and mention this release note. |
