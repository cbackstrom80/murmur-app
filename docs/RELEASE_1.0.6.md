# MURMUR 1.0.6 — Interstellar preset install fix

Fixes a **macOS pkg upgrade bug** where the **Interstellar** factory bank (100 cinematic presets) could be missing after installing 1.0.5 over an earlier MURMUR release — even though the `.pkg` payload contained the files.

## What was broken

- Upgrading from 1.0.4 → 1.0.5 left `~/Library/Application Support/MURMUR/Presets/factory/` with only the original **800** core presets (Basses, Leads, Pads, Sequences, Ambient).
- **`Interstellar/`** (100 `.pw8` files), plus **Warp/** and **Templates/** demo banks, were not merged into an existing factory tree.
- The plugin browser could not filter **interstellar** because the files were never on disk.

## Fix in 1.0.6

- **Dual-path preset staging** in the installer: factory presets ship to both `Presets/factory/` and `Presets/.murmur-factory-staging/`.
- **`postinstall` rsync** merges staging into the live factory folder on every install — guarantees new subfolders land on upgrade.
- **Build-time guard**: `build_release_pkg.sh` fails if Interstellar count &lt; 100 before the `.pkg` is built.
- **Browser facet fix**: category chips match lowercase metadata (`interstellar`) while displaying **Interstellar**.

**900 factory presets** total (800 core + 100 Interstellar), plus Warp/Templates demos.

## Install

Download **`MURMUR-1.0.6-macOS-arm64.pkg`** (or `.dmg`) — double-click, quit Logic, rescan AU, browse **Interstellar** presets.

## Manual fix (without waiting for installer)

```bash
rsync -a /path/to/murmur-app/content/presets/factory/ \
  "$HOME/Library/Application Support/MURMUR/Presets/factory/"
```

Or re-run the 1.0.6 `.pkg` — postinstall rsync handles the merge automatically.

## Unchanged from 1.0.5

- Weeks 4–7 warp suite, DESIGN FX/mod UX, Interstellar preset content and tests
- MURMUR Audio Unit, wavetable library, Logic + MP11SE docs
