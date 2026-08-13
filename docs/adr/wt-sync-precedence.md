# ADR: Graph SYNC vs Wavetable Internal Sync Precedence

**Status:** Accepted (Aug 2026)  
**Context:** [DESIGN_AND_WARPS_PLAN.md](../DESIGN_AND_WARPS_PLAN.md) decision D2

## Decision

When both an algorithm-graph `EdgeType::Sync` edge and wavetable internal sync (`wtSyncRatio` / `wtSyncAmount`) are active on the same operator:

1. **Graph SYNC wins on phase-wrap events.** If a feed-forward SYNC edge fires (source oscillator wrapped last sample), the destination operator's phase accumulator resets on the next sample — same as today for all engine types.
2. **Wavetable internal sync affects read phase only.** `wtSync*` warps how the table is read within the current carrier cycle; it does not override or suppress graph SYNC resets.
3. **`didWrapThisSample()` semantics are unchanged.** Graph SYNC listens to the source node's wrap flag; wavetable internal sync must not set or clear that flag independently of the carrier phase accumulator.

## Rationale

Graph topology is the authoritative structural mod routing layer. Internal wt sync is a timbral warp on the Wavetable engine path (Serum-class "sync" knob), not a second parallel sync bus. Letting wt sync override graph SYNC would make edge-based FM/sync patches non-deterministic and break existing `SyncEdgeTests`.

## Consequences

- Week 5 warp integration must apply wt sync **before** table read, **after** external phase mod, and **before** graph SYNC reset is applied to the accumulator.
- DESIGN graph editor documentation should state that SYNC edges remain the hard reset mechanism.
- See also [ALGORITHM_GRAPH.md](../ALGORITHM_GRAPH.md) SYNC row in the execution table.
