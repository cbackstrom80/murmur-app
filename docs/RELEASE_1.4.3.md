# MURMUR 1.4.3 — Cadillac Ben Release

Customer-ready build for **Apple Silicon Macs** (M1/M2/M3/M4), **macOS 13+**, **Logic Pro**.

This is the polished Ben installer: full content bundle, iPad-style PLAY layout, Preset Explorer, and a drag-and-readme DMG.

## Install (Ben)

1. Download **`MURMUR-1.4.3-macOS-arm64-full.dmg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases)
2. Open the DMG → read **`1 — READ ME FIRST.txt`**
3. Double-click **`Install MURMUR.pkg`**
4. If Gatekeeper blocks it: **right-click → Open → Open**
5. **Quit Logic completely**, reopen
6. **Settings → Plug-in Manager → Reset & Rescan Selection**
7. Software Instrument track → **AU Instruments → Murmur → MURMUR**
8. Open presets: **BROWSE** button, click the preset name, or **⌘B**

Optional: drag **MURMUR.app** from the DMG to **Applications** to test standalone.

## What's included

| Item | Path after install |
|------|-------------------|
| Audio Unit (Logic) | `~/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| Standalone app | `~/Applications/MURMUR.app` (full installer) |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/MURMUR.vst3` (full installer) |
| Factory presets (1800+) | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Design FX presets | `~/Library/Application Support/MURMUR/design-fx/` |
| Docs + release notes | `~/Library/Application Support/MURMUR/Docs/` |

## What's new in 1.4.3

- **Cadillac installer** — DMG with readme, install pkg, checksums, hard-fail auval gate
- **iPad-style PLAY view** — scope + master output deck, 6 macro knobs, mode pills
- **Preset Explorer** — BROWSE button, preset pill click, ⌘B shortcut; mood/genre/tag filters
- **Crash fix** — editor startup recursion guard
- **Install script fix** — always picks the newest built AU (no more stale 1.4.0)

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Logic shows old version (1.4.0) | Quit Logic → Plug-in Manager → Reset & Rescan → reinstall pkg |
| Logic crash on load | Reinstall 1.4.3 full pkg; confirm version in plug-in footer |
| Gatekeeper blocks pkg | Right-click → Open |
| No presets | Check `~/Library/Application Support/MURMUR/Presets/factory/` for `.pw8` files |
| Can't open preset browser | Click **BROWSE** in header or press **⌘B** |

## Build (maintainers)

```bash
scripts/release_gate.sh 1.4.3
# Output: dist/MURMUR-1.4.3-macOS-arm64-full.{pkg,dmg,SHA256SUMS.txt}
```

Push tag `v1.4.3` to trigger CI release workflow.
