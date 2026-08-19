# MURMUR 1.6.0 — Branded Splash, License Activation, Installer Polish

Customer-ready build for **Apple Silicon Macs** (M1/M2/M3/M4/M2 Ultra), **macOS 13+**, **Logic Pro**.

## Install (Ben)

1. Download **`MURMUR-1.6.0-macOS-arm64-full.dmg`** from [GitHub Releases](https://github.com/cbackstrom80/murmur-app/releases)
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

## What's new since 1.5.0

- **Branded splash screen** — real kelp-forest + whale artwork on launch, fixed-duration progress bar with honest phase labels (not a fabricated loading percentage).
- **License activation** — a new ACTIVATE YOUR ENGINE screen accepts a license key (generated at murmur-web's `/keys` page) and syncs a starter patch library from murmur-web's live backend into your local preset folder. Optional — SKIP FOR NOW dismisses it with no consequence; nothing is paywalled yet.
- **Installer polish** — the DMG now opens to a real branded Finder window (same kelp/whale artwork) instead of a plain file list, with one clear action (`Install MURMUR.pkg`) plus a `Documentation/` folder instead of six loose text files.
- **`.pw8` → `.murmur` packaging fix** — the release script's factory-preset bundling logic was still matching only the pre-rebrand `.pw8` extension; fixed to match both, so this and all future releases bundle the real 1,129-preset factory library (including the full 175-file Interstellar bank) rather than a smaller placeholder set.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Logic says "incompatible" | Delete old MURMUR.component, reinstall 1.6.0 pkg, rescan |
| Blank plugin window | Reinstall pkg (content missing); try Standalone app first |
| Gatekeeper blocks pkg | Right-click → Open |
| No presets in explorer | Confirm `~/Library/Application Support/MURMUR/Presets/factory/` has `.murmur` (or legacy `.pw8`) files |

## Build (maintainers)

```bash
scripts/build_release_pkg.sh --dmg --full 1.6.0
# Output: dist/MURMUR-1.6.0-macOS-arm64-full.{pkg,dmg}
```

Push tag `v1.6.0` to trigger the CI release workflow.
