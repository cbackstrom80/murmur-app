# MURMUR 1.6.2 — Resonator Fix & Filter 2 Response Variety

Customer-ready build for **Apple Silicon Macs** (M1/M2/M3/M4/M2 Ultra), **macOS 13+**, **Logic Pro**.

## Install (Ben)

1. Download **`MURMUR-1.6.2-macOS-arm64-full.dmg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases)
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

## What's new since 1.6.1

A real DSP-quality pass, prompted by a systematic engine review — no new UI. Everything below was verified end-to-end (real render measurements, real test coverage), not just "it builds."

- **Fixed the Resonator engine (Engine Type 8) producing near-silent output at moderate/high `decay` settings.** Any patch relying on it as the *only* audible operator — a genuinely common case for "resonant," "physically-modeled," or "plucked-string" briefs — could render at 1/100th to 1/1000th its intended level, or effectively silent. Root cause: the exciter burst was a fixed ~6ms regardless of how narrow-band (high-Q) the mode filters were, and overall output level was never actually calibrated. Both fixed; output now stays consistently audible across the full `decay` range. 29 factory presets that use this engine at meaningful `decay` values sound different (louder, correctly) as a direct result — everything else is unaffected.
- **Filter 2 (the "character" ladder filter) now has real highpass and bandpass responses**, not just lowpass. A new `modeMorph` knob (LP → BP → HP, same convention as Filter 1's own morph) derives them from the existing 4-pole cascade — same character/drive, more usable range. Defaults to lowpass-only, so existing patches/presets sound exactly the same unless you turn the new knob.
- **Regression coverage added** so both of the above can't silently break again: a new automated per-engine "is this actually audible" check now runs alongside every build, plus dedicated Filter 2 response tests.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Logic says "incompatible" | Delete old MURMUR.component, reinstall 1.6.2 pkg, rescan |
| Blank plugin window | Reinstall pkg (content missing); try Standalone app first |
| A saved patch using the Resonator engine sounds louder than before | Expected — see the Resonator fix above. The new level is the *correct* one; if a specific patch now clips or feels too hot, pull its level/mix down manually. |
