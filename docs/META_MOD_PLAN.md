# Meta-Modulation — Deferred Engine Work

**Date:** 2026-08-14  
**Status:** Documented stub — not implemented in v1.1.1  
**Related:** `docs/ASM_MACRO_KOINS_RESEARCH.md`, `docs/MODULATION.md`, `docs/HORIZON2.md`

---

## Concept

**Meta-modulation** lets a macro (or other mod source) control the **depth** of an existing mod route — Hydrasynth-style “macro → mod amount” without duplicating destinations.

Example: Macro2 opens filter cutoff *and* increases LFO1→Cutoff route depth so motion intensifies with BLOOM.

---

## Current gap

| Piece | Status |
|-------|--------|
| `ModRoute.amount` per voice/layer | Shipped |
| Macro → destination via `modRoutes` | Shipped |
| Macro → **another route's amount** | **Not implemented** |
| MCP / preset schema for meta routes | Not defined |

`ModMatrixExecutor` applies routes once; there is no second pass for “modulate mod depth.”

---

## Proposed schema (v4 candidate)

Optional `metaRoutes` on `layerA` / `layerB`:

```jsonc
{
  "source": 21,
  "targetRouteIndex": 3,
  "amount": 0.5,
  "scope": 0
}
```

Or extend `ModDestination` with `ModRouteDepth` + `targetIndex` = route index.

---

## Executor sketch

```
1. Resolve base route depths from patch
2. Apply metaRoutes: depth' = depth + metaContribution(source, amount)
3. Run standard mod matrix with depth'
```

Requires stable route indexing or route IDs in JSON.

---

## Why deferred

- Needs route identity in presets (today routes are an ordered array)
- UI: destination picker for “which route to meta-mod”
- Test matrix: meta + dissemination + morph baselines

**MCP today:** use `set_macro_koin` with extra destinations instead of meta-mod until executor ships.

---

## Verify when implemented

1. Patch with LFO→Cutoff + Macro2→LFO depth — sweep Macro2, motion depth follows.
2. No feedback loop when meta source equals route source.
3. Dissemination: per-note macro snapshot does not retroactively change frozen route depths (policy TBD).
