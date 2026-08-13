# MURMUR 1.0.2 — Product gap plan + docs in installer

Ships the **product gap plan** and keeps the full product documentation set in the macOS installer.

## New in 1.0.2

- **PRODUCT_GAP_PLAN.md** bundled under `~/Library/Application Support/MURMUR/Docs/` (roadmap: DESIGN mode, filters, warps, sampler, analyzers, mod UX)
- Product docs index links to the gap plan from `Docs/product/README.md`
- Version bump 1.0.1 → 1.0.2

## Install

Download **`MURMUR-1.0.2-macOS-arm64.pkg`** — same Ben MVP flow: double-click, quit Logic, rescan AU, play.

After install:

```
~/Library/Application Support/MURMUR/Docs/PRODUCT_GAP_PLAN.md
~/Library/Application Support/MURMUR/Docs/product/README.md
```

## Unchanged from 1.0.1

- MURMUR Audio Unit, factory presets, wavetables
- Full product doc set (`Docs/product/`), Logic + Kawai MP11SE guides
