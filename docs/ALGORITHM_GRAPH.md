# Algorithm Graph

**IMPLEMENTED** (AUDIO/PHASE_MOD/FREQUENCY_MOD/AMPLITUDE_MOD/RING_MOD/SYNC/FEEDBACK
edge types, full compiler, full executor). This is the core differentiator called
out in the product vision, and it's real working code in this pass, not scaffolding.

## Model

Every layer has exactly 8 nodes (`core::kNodesPerLayer`). A node
(`algorithm::AlgorithmNode`) has a stable `NodeId` (0-7), an `EngineType`, and an
`isOutput` flag. Nodes are connected by typed, directed edges
(`algorithm::AlgorithmEdge`): `source`, `destination`, `type`, `amount`.

```cpp
enum class EdgeType : uint8_t {
    Audio, PhaseMod, FrequencyMod, AmplitudeMod, RingMod, Sync, Feedback,
};
```

## Compiler

`algorithm::AlgorithmGraphCompiler::compile(AlgorithmGraphDefinition, CompiledAlgorithm&)`
is the *only* path by which a graph reaches the audio thread. It:

1. Validates node IDs are exactly `{0..7}` with no duplicates (`WrongNodeCount` /
   `DuplicateNodeId` / `InvalidNodeId`).
2. Validates every edge references valid node IDs (`InvalidEdgeReference`) and stays
   within `core::kMaxAlgorithmEdges` (`TooManyEdges`).
3. **Eliminates zero-value edges** (`|amount| < 1e-6`) at compile time -- they never
   reach the executor.
4. Splits edges into feed-forward (`Audio`/`PhaseMod`/`FrequencyMod`/`AmplitudeMod`/
   `RingMod`/`Sync`) and feedback (`Feedback`) groups.
5. **Topologically sorts** the feed-forward subgraph via Kahn's algorithm. If it
   contains a cycle, compilation fails with `FeedForwardCycle` -- arbitrary
   same-sample cycles are never allowed to execute. `Feedback`-typed edges are
   exempt from this check (see "Feedback Policy" below) and can freely form loops,
   including self-loops.
6. Requires at least one output node (`NoOutputNodes` otherwise).
7. Produces an immutable `CompiledAlgorithm`: `executionOrder`, `feedForwardEdges`,
   `feedbackEdges`, `outputNodes`, `nodeEngines`.

`render::Engine::loadPatch()` calls this on the control/UI thread. **If compilation
fails, the engine does not leave the audio thread with an invalid graph** -- it falls
back to `AlgorithmGraphDefinition::makeDefaultParallel8()` (guaranteed to compile)
and reports the failure via `getLastCompileStatus()`. See
`tests/unit/AlgorithmGraphCompilerTests.cpp` for the full validation matrix (cycle
rejection, feedback-loop acceptance, self-feedback, missing outputs, duplicate IDs,
out-of-range references, zero-edge elimination).

## Execution (audio thread)

`algorithm::AlgorithmExecutor::processSample()` (`pw8/algorithm/AlgorithmExecutor.hpp`)
runs once per sample per voice, over `compiled.executionOrder`:

| Edge type | Effect on destination |
|---|---|
| `AUDIO` | `output += source.output * amount` |
| `PHASE_MOD` | oscillator phase offset by `source.output * amount` (cycles) |
| `FREQUENCY_MOD` | oscillator frequency offset by `source.output * amount` (Hz) |
| `AMPLITUDE_MOD` | `output *= (1 + source.output * amount)` |
| `RING_MOD` | `output *= (source.output * amount)` -- stacks multiplicatively with `AMPLITUDE_MOD` if both target the same node (a deliberate MVP simplification: true independent AM/RM busses are a future refinement) |
| `SYNC` | destination oscillator phase reset to 0 the sample *after* the source wraps |
| `FEEDBACK` | see below |

Because `AMPLITUDE_MOD`/`RING_MOD`/`PHASE_MOD`/`FREQUENCY_MOD`/`AUDIO` edges are all
feed-forward and the executor walks nodes in topological order, a destination node
always sees its modulators' *finished* per-sample output before it renders --
there's no same-sample race.

## Feedback Policy

`FEEDBACK` edges are the only ones allowed to form a loop (including a node feeding
back into itself, the classic FM-operator self-feedback case). They are *always*
one-sample-delayed: the executor reads `states[source].lastOutput` (the previous
sample's finished value), never the current sample's in-progress value. Before use,
the fed-back signal is:

1. Scaled by `amount`, clamped by the compiler to `[-2, 2]` (feedback gain guard).
2. Soft-saturated with `tanh()`.
3. Passed through `dsp::flushIfNotFinite()`.

then applied as a phase modulation to the destination. This is deliberately the
*only* modulation type `FEEDBACK` edges express in this pass (real hardware/software
FM feedback is almost always phase feedback) -- see `content/algorithms/feedback_bell.json`
and `tests/regression/RenderSanityTests.cpp`'s "self-feedback algorithm" case, which
asserts finite, bounded output even with an aggressive `amount = 1.5` self-feedback edge.

Every per-node output is additionally clamped to `[-16, 16]` and passed through
`flushIfNotFinite()` after rendering, regardless of edge configuration -- this is the
"every risky nonlinear stage must reject non-finite state" rule from the master spec,
applied without per-sample cost anywhere it isn't needed (the checks are on the
already-computed per-node output, not sprinkled through the oscillator math itself).

## Algorithm Library

`content/algorithms/*.json` -- 10 original, data-driven templates (`parallel_8`,
`serial_8`, `dual_stack`, `triple_carrier`, `classic_four_operator`, `feedback_bell`,
`cross_mod`, `ring_network`, `carrier_cluster`, `spectral_exciter`). See
[content/algorithms/README.md](../content/algorithms/README.md) for the full
description of each and the raw integer-code reference table. These load directly
as a patch layer's `algorithm` field -- `PatchSerializer` only reads `nodes`/`edges`
from the file, so the human-readable `id`/`name`/`description` fields are ignored at
load time but kept for documentation/tooling.

## Algorithm Morph

**PLANNED** (Phase 9). `AlgorithmMorph`'s same-topology (interpolate edge
gain/operator params) and different-topology (dual-compile + equal-power output
blend) cases are not implemented in this pass.

## Graph Inspector

`pw8-graph inspect <preset.pw8>` (`tools/graph_inspector/`) prints a compiled
algorithm's execution order and both edge groups in a readable form, e.g.:

```
Layer A:
  Nodes (execution order):
    OP2 [classic]
    OP1 [classic]
    OP0 [classic]  -> OUT
  Feed-forward edges:
    OP2 --PM(0.8)--> OP1
    OP1 --PM(1)--> OP0
  Feedback edges:
    OP1 ==FB(0.6)==> OP1  [one-sample delayed]
```
