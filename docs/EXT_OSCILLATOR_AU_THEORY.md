# EXT Oscillator via AU Sidechain — Theory

**Status:** Sidechain **envelope follower MVP shipped in v1.1.0** (mod matrix source + UI badge). Full **EXT engine type** remains research / future work.

**Question:** Could an `EXT` (external audio) oscillator engine type be available in Logic Pro AU mode, letting the user route e.g. a "Vocal BUS" into operator 0 instead of a wavetable?

---

## Feasible?

**Partial — yes for Logic AU; no or degraded elsewhere without extra work.**

| Host / format | Verdict |
|---|---|
| **Logic Pro + AU (`aumu`)** | **Yes.** Logic exposes a sidechain picker on AU instruments that declare an extra input bus (ES2, Sculpture, EVOC PolySynth do this today). User selects a bus or track as the sidechain source. |
| **VST3** | **Partial.** JUCE can expose the first input as an **Aux** sidechain via `Vest3ClientExtensions::getPluginHasMainInput() → false`. Host support and UX differ by DAW; not Logic-specific. |
| **Standalone** | **No** (as described). No "Vocal BUS" unless the app adds a live audio-input device path — a different feature. |
| **AU in other DAWs** | **Varies.** Sidechain routing exists but UX is not standardized like Logic's bus model. |

MURMUR is already configured as an AU instrument (`kAudioUnitType_MusicDevice`, `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`) in `plugin/CMakeLists.txt`. **v1.1.0** adds a stereo **Sidechain** input bus (AU builds only) and an envelope follower exposed as **`ModSource::Sidechain`** in the mod matrix. Logic can route a bus or track into the sidechain picker; the follower level (0..1) modulates destinations like any other source. A **Sidechain** badge in PLAY performance view pulses when input is active.

Full **EXT** (replace operator 0 audio with sidechain samples) is **not** implemented yet — see [Proposed architecture](#proposed-architecture-ext-on-operator-0-au) below.

---

## Shipped in v1.1.0 (sidechain follower MVP)

| Piece | Where |
|-------|--------|
| AU sidechain input bus | `MurmurProcessor::makeProcessorBuses()` — `"Sidechain"` stereo input |
| Envelope follower | `engine/include/pw8/dsp/SidechainFollower.hpp` |
| Mod matrix source | `ModSource::Sidechain` → any destination (e.g. Quasar distance) |
| Engine hook | `Engine::setSidechainLevel()` in master-bus mod sources |
| UI | PLAY performance **Sidechain (AU)** badge when routed or active |
| MCP | `sidechain` in `patch_schema.py` mod source enum |

**Logic setup:** Instrument track → MURMUR → sidechain menu → pick vocal bus or track. Add a mod route **SIDECHAIN →** target (MOD tab or patch JSON). Envelope follows sidechain RMS; no replacement of internal oscillators.

**Deferred:** `EngineType::External` on operator 0 (see below).

---

## Current architecture (baseline)

### Operator / engine selection

- Eight synthesis engines are defined in `engine/include/pw8/algorithm/AlgorithmTypes.hpp` (`EngineType`: Classic … Resonator). There is **no EXT type**.
- Per-operator engine is stored in patch data (`patch::OperatorPatch::engine`) and exposed to the host as APVTS choice `opN_Engine` (0..7) via `plugin/src/state/PluginState.cpp`.
- UI engine pills (`EngineIconGrid`, `OperatorEditorPanel`) switch on the same enum.
- `op::OperatorState::render()` in `engine/include/pw8/operator/OperatorNode.hpp` switches on `params.engine`. Wavetable and Granular read from `wavetableTables[i]` (loaded at patch load from `wavetableId`); other engines are fully internal.

### Graph execution

- `algorithm::AlgorithmExecutor::processSample()` calls `states[i].render(..., wavetableTables[i], ...)` per node in execution order.
- `render::Engine::process()` drives voices; each voice runs the compiled graph sample-by-sample. External audio is **not** in this path today.

### Plugin I/O (JUCE)

- `MurmurProcessor` constructor: `BusesProperties().withOutput("Output", stereo, true)` only — **zero input buses**.
- `processBlock()` clears the output buffer, ignores any would-be input, and calls `engine->process(view, midi)`.
- No overrides of `isBusesLayoutSupported()`, `getBusCount()`, or sidechain-related APIs.

---

## Proposed architecture: EXT on operator 0 (AU)

### Concept

Add `EngineType::External` (UI label **EXT**). When active on **operator 0 only**, that node's per-sample output comes from the **host sidechain buffer** instead of internal DSP.

```
Logic Vocal BUS ──send──► Bus N ──sidechain──► MURMUR input bus 0
                                                      │
                                                      ▼
                              processBlock: getBusBuffer(..., isInput=true, busIndex=0)
                                                      │
                                                      ▼
                              Engine::process(..., extAudioView)  // stereo or mono → float
                                                      │
                                                      ▼
                              AlgorithmExecutor op0: EXT → read extAudio[sampleIndex]
                                                      │
                                                      ▼
                              Rest of graph (FM, filters, FX) unchanged
```

### Operator 0 only

Restricting to the first engine (node 0) is reasonable and reduces scope:

- One sidechain tap, one place in the graph where "replace oscillator with bus audio" is defined.
- Avoids ambiguity when multiple EXT ops would share one bus.
- UI can hide EXT on operators 1..7 unless product scope expands later.
- `AlgorithmGraphCompiler` / default templates need a rule: EXT only valid on `NodeId(0)` (compile-time or runtime guard).

### Semantics (design choices to nail down)

- **Level / pan:** Apply `OperatorParams::level` as today.
- **Key track / ratio:** Likely **ignored** (audio is already pitched by the source) or optionally used only for filtering/mod depth — not as a repitch of the buffer.
- **Phase mod / sync edges:** EXT has no meaningful phase accumulator; SYNC-from-EXT and phase-mod **into** EXT need explicit no-op or "pass-through sample" rules.
- **Multi-voice:** All voices read the **same** sidechain sample at each time index (correct for a shared bus source).
- **Missing sidechain:** Silence on op 0 (same as null wavetable today).

---

## Logic Pro UX — routing "Vocal BUS" to MURMUR

Logic does **not** sidechain track-to-track directly; it uses **internal buses**.

1. **Create / name a bus** for vocals (e.g. rename Bus 1 to "Vocal BUS" in the mixer).
2. **On the vocal track(s):** add a **Send** to that bus (prefader or postfader as desired). Set send level; mute the aux channel's main output or set aux output to **No Output** if you only want sidechain feed, not audibility.
3. **Insert MURMUR** on a **software instrument track** (standard MIDI instrument slot).
4. **Open MURMUR's plugin window.** After the plugin exposes a sidechain input, Logic shows a **Side Chain** menu (top-right of the plugin frame, same as Compressor / ES2).
5. **Choose the bus** (e.g. "Vocal BUS" / Bus 1) from that menu.
6. **In MURMUR:** set **operator 0** engine to **EXT**. Play MIDI on the instrument track; operator 0's contribution is the routed vocal audio, modulated/routed by the rest of the patch.

**AU v2 vs v3:** JUCE builds a classic **AU v2** `.component` for MURMUR. Logic Pro uses AU v2 components natively; sidechain on Music Devices is long-established on v2 (Apple's ES2, etc.). No separate AUv3 app extension is required for this workflow.

---

## JUCE changes needed (theoretical)

### 1. Bus layout (AU + optionally VST3)

```cpp
// Pseudocode — not implemented
BusesProperties()
    .withOutput("Output", AudioChannelSet::stereo(), true)
#if JucePlugin_Build_AU || JucePlugin_Build_VST3
    .withInput("Sidechain", AudioChannelSet::stereo(), true)  // bus index 0 for AU sidechain in Logic
#endif
;
```

- Override **`isBusesLayoutSupported()`**: main output stereo (or mono); sidechain mono or stereo enabled/disabled; reject layouts that would turn MURMUR into an audio-through effect on the instrument channel.
- **VST3:** inherit `VST3ClientExtensions`, override `getPluginHasMainInput() const → false` so the input is **Aux**, not main — avoids host mixing channel audio with synth output (JUCE forum / `juce_VST3ClientExtensions.h` pattern for vocoder-style synths).
- **Standalone:** omit sidechain bus (or stub silent input) so builds stay simple.

### 2. `processBlock`

- Obtain sidechain: `getBusBuffer(buffer, true, 0)` (verify index against `getBusCount(true)` after layout negotiation).
- Pass pointer + channel count + `numSamples` into `Engine::process()` (new optional parameter or small `ExternalAudioView` struct).
- Do **not** mix sidechain into the main output buffer; only operator 0 consumes it.

### 3. Core engine path

- Extend `Engine::process()` → `Voice::renderSample()` → `AlgorithmExecutor::processSample()` → `OperatorNode::render()` with per-block external audio view + current sample index.
- Add `EngineType::External` branch: output = `(L+R)*0.5` or per-channel rule for stereo sidechain.

### 4. AU validation

- Re-run **`auval`** and **`pluginval`** after bus changes (current CI assumes output-only instrument).
- Confirm Logic shows sidechain menu and that silent sidechain does not fail render tests.

### 5. UI / parameters

- Show **EXT** in engine picker **only when** `JucePlugin_Build_AU` (or runtime host detection) **and** `selectedNode == 0`.
- If user loads a patch with EXT on op > 0, fall back to Classic or clamp at compile/load.
- `isEngineImplemented(External)` gating in `AlgorithmTypes.hpp`.

---

## Limitations

| Area | Limitation |
|---|---|
| **Standalone / VST3** | No Logic-style "Vocal BUS" unless user adds VST3 sidechain in a supporting DAW or Standalone mic/line input. |
| **Logic channel count** | Sidechain is **mono or stereo only** (not surround). |
| **Latency / PDC** | Vocal path may include sends, plugins on vocal track, and bus latency; sidechain is **not** automatically time-aligned with internal osc phase — may need manual delay compensation on the send for tight FM/sync use cases. |
| **Only op 0** | By design; other operators remain internal engines. |
| **Not a sampler** | Continuous stream per buffer, not note-sliced grains (unless combined with Granular-like processing later). |
| **Graph edges** | FM/PM/Sync into/out of EXT need explicit DSP policy. |
| **Preset portability** | EXT patches are meaningful only when host sidechain is connected; document in preset metadata. |

---

## Alternatives

1. **Sidechain as modulator only (audio follower)**  
   Keep internal oscillators; use sidechain envelope to modulate level, filter, FM index, etc. Smaller change (no new `EngineType`), but **does not** replace wavetable audio with vocals.

2. **External instrument on aux + resample / manual workflow**  
   User records vocal to audio, loads as wavetable — works today via `wavetableId` / file load, not live.

3. **Full EXT engine type (proposed)**  
   Operator 0 literally outputs sidechain samples — best match for "use Vocal BUS as oscillator source" in Logic AU.

4. **AU MIDI-controlled effect (`aumf`) on instrument channel**  
   Logic can host `aumf` in the instrument slot with audio via sidechain-as-main-input; surround/main-bus rules differ. MURMUR is **`aumu`**; switching subtype is a product/format change, not a small add-on.

---

## Effort estimate

**M (leaning L)** — ~1–2 weeks focused work for a solid AU-first slice:

| Workstream | Size |
|---|---|
| JUCE buses + `processBlock` + layout support + auval/pluginval | S–M |
| `EngineType::External` + render path threading + edge-case policy | M |
| UI gating (op 0, AU only) + preset/compile guards | S |
| Logic manual QA + docs | S |
| VST3 aux sidechain parity (optional) | M |

**S** alone if scope is **audio follower modulator** only (no EXT type).  
**L** if multi-op EXT, repitch/time-stretch, or cross-format parity with automated host tests.

---

## Summary for Curtis

**Yes in theory for Logic AU:** declare a sidechain input bus on MURMUR, read it in `processBlock`, and add an **EXT** engine on **operator 0** that outputs those samples instead of wavetable DSP. Logic's sidechain menu + bus routing is exactly how ES2-style instruments accept "Vocal BUS". **Not** a small toggle — it touches bus layout, the entire sample render path, and validation — but no fundamental AU or graph blocker. Standalone/VST3 would need separate stories; AU-only gating matches "LOGIC AU MODE."
