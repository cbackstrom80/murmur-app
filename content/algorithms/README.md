# Algorithm Library

Data-driven algorithm graph templates. Each file loads directly as the `algorithm`
field of a `.pw8` patch's layer (`layerA.algorithm` / `layerB.algorithm`) -- the
top-level `id`/`name`/`description` fields here are for humans/tooling and are
ignored by `PatchSerializer` (only `nodes` and `edges` are read).

See [docs/ALGORITHM_GRAPH.md](../../docs/ALGORITHM_GRAPH.md) for the full model.
Quick reference for the raw integer codes used in these files:

**`engine`** (`AlgorithmNode.engine` / `algorithm::EngineType`)

| int | name         | status        |
|-----|--------------|---------------|
| 0   | classic      | IMPLEMENTED   |
| 1   | wavetable    | PARTIAL       |
| 2   | fm_pm        | PLANNED       |
| 3   | additive     | PLANNED       |
| 4   | phase_shape  | PLANNED       |
| 5   | granular     | PLANNED       |
| 6   | noise_chaos  | PLANNED       |
| 7   | resonator    | PLANNED       |

**`type`** (`AlgorithmEdge.type` / `algorithm::EdgeType`)

| int | name          | semantics                                                |
|-----|---------------|-----------------------------------------------------------|
| 0   | AUDIO         | destination.output += source.output * amount              |
| 1   | PHASE_MOD     | destination phase offset by source.output * amount (cycles)|
| 2   | FREQUENCY_MOD | destination frequency offset by source.output * amount (Hz)|
| 3   | AMPLITUDE_MOD | destination.output *= (1 + source.output * amount)         |
| 4   | RING_MOD      | destination.output *= (source.output * amount)             |
| 5   | SYNC          | destination phase reset to 0 the sample after source wraps |
| 6   | FEEDBACK      | one-sample-delayed, soft-saturated phase modulation; only edge type allowed to form a loop |

## Templates

| File | Character |
|---|---|
| `parallel_8.json` | 8 independent carriers, no modulation. Baseline / stress test. |
| `serial_8.json` | Deep single-chain PM stack, OP7 -> ... -> OP0 -> OUT. |
| `dual_stack.json` | Two independent groups of 4, both to output. |
| `triple_carrier.json` | Three PM'd carriers plus two plain carriers. |
| `classic_four_operator.json` | Two independent 2-op PM stacks; demonstrates partial node usage. |
| `feedback_bell.json` | Inharmonic self-feedback modulator chain -- metallic/bell timbres. |
| `cross_mod.json` | Two oscillators cross-modulating (one feed-forward, one FEEDBACK-delayed). |
| `ring_network.json` | Three carriers each ring-modulated by a dedicated modulator. |
| `carrier_cluster.json` | Same shape as `parallel_8`; detuning is a patch/operator-level concern. |
| `spectral_exciter.json` | Multi-source hub + modulator; forward-looking template for the resonator engine. |
