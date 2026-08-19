# MURMUR 1.4.0 — Obsidian UI + Preset Explorer (Ben release)

Customer-ready build for **Apple Silicon Macs** (M1/M2/M3/M4/M2 Ultra), **macOS 13+**, **Logic Pro**.

Fixes the blank/incompatible experience reported on the 1.2.0 GitHub download: this release is **arm64-native**, bundles **all factory content**, and ships the complete **Obsidian UI** with **Preset Explorer** modal.

## Install (Ben)

1. Download **`MURMUR-1.4.0-macOS-arm64-full.dmg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases)
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
| Factory presets (1000+) | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Design FX presets | `~/Library/Application Support/MURMUR/design-fx/` |
| Docs | `~/Library/Application Support/MURMUR/Docs/` |

## What's new since 1.2.0

- **Obsidian UI** — PLAY / DESIGN / COMPACT modes, engine grid, FX rack, labs
- **Preset Explorer** — Figma modal popup (categories, search, favorites, load)
- **FX glyph sprite sheet** — distinct icons per effect type
- **macOS 13 minimum** — explicit deployment target for Apple Silicon
- **Full content bundle** — presets, wavetables, design-FX JSON in installer

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Logic says "incompatible" | Delete old MURMUR.component, reinstall 1.4.0 pkg, rescan |
| Blank plugin window | Reinstall pkg (content missing); try Standalone app first |
| Gatekeeper blocks pkg | Right-click → Open |
| No presets in explorer | Confirm `~/Library/Application Support/MURMUR/Presets/factory/` has `.pw8` files |

## Build (maintainers)

```bash
scripts/build_release_pkg.sh --dmg --full 1.4.0
# Output: dist/MURMUR-1.4.0-macOS-arm64-full.{pkg,dmg}
```

Push tag `v1.4.0` to trigger CI release workflow.
