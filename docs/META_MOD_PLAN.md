# Meta-Modulation — Engine Work

**Date:** 2026-08-14 (plan), implemented since — see "Current gap" below.  
**Status:** Implemented in the engine (`patch::MetaModRoute`, `ModMatrixExecutor`, JSON round-trip). **No UI or MCP exposure yet** — see "Why deferred" below, now reduced to that one remaining gap.  
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
| Macro → **another route's amount** | **Shipped** — `patch::MetaModRoute` (`engine/include/pw8/patch/Patch.hpp`), executed by `ModMatrixExecutor::apply()` (`engine/include/pw8/modulation/ModMatrixExecutor.hpp`), threaded through `Voice::renderSample`, round-tripped through patch JSON (`engine/src/patch/PatchSerializer.cpp`) |
| MCP / preset schema for meta routes | Preset JSON schema shipped (see below); **no MCP tool exposure, no plugin UI** |

`ModMatrixExecutor::apply()` does take a real second pass over `metaRoutes`, applying each one's `amount` to the target route's depth before running the standard mod matrix — this doc's original claim that there's no second pass is no longer accurate.

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
