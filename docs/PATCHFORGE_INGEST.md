# Patchforge ingest

Headless pipeline that turns real `.pw8` patches into Patchforge preview assets and a typed catalog JSON.

## What it produces

Given a manifest under `scripts/patchforge/manifests/`, the ingest script writes into the Patchforge `public/` tree:

| Output | Purpose |
|--------|---------|
| `catalog.generated.json` | Pack metadata, per-patch quality scores, waveform peaks |
| `audio/ingest/<pack-slug>/*.wav` | Engine-rendered preview audio (1.6s hold + release tail) |

Patchforge loads `catalog.generated.json` at build time. If the file is empty or missing packs, the storefront falls back to the small mock VITAL demo catalog.

## Prerequisites

From the MURMUR repo root:

```bash
cmake --build --preset dev
```

This must produce `build/dev/tools/murmur-render`.

## Run ingest

```bash
python3 scripts/patchforge_ingest.py \
  --manifest scripts/patchforge/manifests/mvp.json \
  --out ../patchforge/public
```

### Flags

- `--no-validate` — skip extra low/mid/high validation renders (faster CI / dev loops). Quality scores still use the main preview render.

### MVP manifest

`scripts/patchforge/manifests/mvp.json` defines three MURMUR packs (18 patches total):

- **starfighter-starter-cache** (free)
- **signal-cinema-vol-1**
- **dark-techno-basses-vol-1**

Each patch entry references a repo-relative `.pw8` path and optional preview MIDI notes.

## Patchforge dev loop

In the Patchforge repo:

```bash
npm run ingest   # re-render from MURMUR (see package.json)
npm run dev      # http://localhost:5173
```

After ingest, hard-refresh the browser to pick up new WAV files and catalog JSON.

## Quality scoring

Scores (0–100) are derived from render metrics:

- NaN/Inf in output → heavy penalty
- Peak too hot or too quiet → penalty
- RMS out of useful range → penalty
- With validation enabled: low/mid/high note probes must also pass sanity checks

Pack quality score is the mean of patch scores.

## Adding a pack

1. Add `.pw8` files under `content/presets/` (or factory banks).
2. Append a pack block to a manifest JSON.
3. Re-run ingest.
4. Commit generated assets in Patchforge if you want static hosting without re-rendering.

## CI hook (optional)

`scripts/mvp_check.sh` can be extended with a fast ingest smoke:

```bash
python3 scripts/patchforge_ingest.py \
  --manifest scripts/patchforge/manifests/mvp.json \
  --out /tmp/patchforge-smoke \
  --no-validate
```

Expect ~18 renders for the MVP manifest (~2–5 minutes depending on machine).
