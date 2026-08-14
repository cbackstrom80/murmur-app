# Morph KOIN Specification (Horizon 2 metadata / Horizon 3 DSP)

**Date:** 2026-08-14  
**Status:** Horizon 2 — schema + MCP metadata **specified**; runtime morph executor **deferred** to Horizon 3  
**Related:** `docs/MUTABLE_INSTRUMENTS_FRAMES_RESEARCH.md`, `docs/ASM_MACRO_KOINS_RESEARCH.md`, `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md`, `docs/PATCH_FORMAT.md`

---

## 1. Concept

A **morph KOIN** is a Frames-inspired performance control: **one knob interpolates between 2–4 named snapshot states (keyframes)**, not a live `modRoutes` fan-out.

| Paradigm | Control | State model | MURMUR analog |
|----------|---------|-------------|---------------|
| **Macro KOIN** (Spread) | Macro1–3 knob | Live sweep via `modRoutes` | PoliMATHS Spread / Hydrasynth macro |
| **Morph KOIN** (Frames) | Single timeline knob | Stored keyframes, interpolated position | Mutable Instruments FRAME knob |
| **Dissemination** | Macro at note-on | Per-voice freeze of one macro value | PoliMATHS Modulation Dissemination |

**Design rule:** Do **not** overload macro KOINS to mean both Spread **and** keyframe morph. Use distinct kinds: `uiFocus.kind: "macro"` vs `uiFocus.kind: "morph"`.

Morph is **orthogonal** to `modRoutes`: keyframes store **base values** (macro levels and/or APVTS param overrides). Macro KOINS still add **deltas** from the current macro axis on top of the morphed baseline (Horizon 3 executor).

---

## 2. Schema shape (`.pw8` JSON)

### 2.1 Top-level `morphKoin` (authoritative data)

Keyframe contents live in a dedicated top-level block — parallel to `macros[]`, not embedded only in `uiFocus`:

```jsonc
"morphKoin": {
  "label": "EVOLVE",
  "description": "TIGHT cathedral pad ↔ wide nebula wash — filter, WT, space.",
  "defaultPosition": 0.35,
  "position": 0.35,
  "curve": "smooth",
  "wrap": false,
  "keyframes": [
    {
      "name": "TIGHT",
      "position": 0.0,
      "macroValues": [0.15, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
      "paramOverrides": {
        "filterCutoffHz": 900.0,
        "layerA.filter1.resonance": 0.12,
        "layerA.operators[0].wavetableFramePosition": 0.08
      }
    },
    {
      "name": "BLOOM",
      "position": 0.45,
      "macroValues": [0.55, 0.35, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    },
    {
      "name": "VOID",
      "position": 1.0,
      "macroValues": [0.85, 0.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
      "paramOverrides": {
        "filterCutoffHz": 14000.0,
        "masterEffects[2].mix": 0.72
      }
    }
  ]
}
```

| Field | Required | Notes |
|-------|----------|-------|
| `label` | Yes | PLAY display name (≤32 chars), e.g. `EVOLVE`, `SCENE` |
| `description` | Recommended | Mission card / agent hint |
| `keyframes` | Yes | **2–4** entries; each must have `name` |
| `defaultPosition` | Yes | Initial timeline position `0..1` (patch default) |
| `position` | Optional | Current performance position; defaults to `defaultPosition` on load if omitted |
| `curve` | Optional | Segment easing: `linear` (default), `smooth` (raised cosine), `step` |
| `wrap` | Optional | If true, interpolate last→first past position 1.0 (Parasite-style loop) |
| `keyframes[].position` | Optional | Explicit timeline slot `0..1`; if omitted, evenly spaced (0, 1/(N−1), …, 1) |
| `keyframes[].macroValues` | Optional | Length-8 array matching `macros[]` indices; omitted slots = no override at that keyframe |
| `keyframes[].paramOverrides` | Optional | Map of dotted APVTS / patch paths → float values (Horizon 3 executor resolves paths) |

**Why not only `uiFocus`?** Keyframes are structured patch data (like `macros[]`), not a single knob binding. `uiFocus` declares **which** morph block appears on the performance surface.

### 2.2 `uiFocus` exposure

When a patch defines `morphKoin`, include **one** morph entry in `uiFocus.knobs`:

```jsonc
"uiFocus": {
  "maxKnobs": 3,
  "knobs": [
    { "kind": "morph", "label": "EVOLVE" },
    { "kind": "macro", "index": 0, "label": "BLOOM" },
    { "kind": "macro", "index": 1, "label": "SPACE" }
  ]
}
```

- `kind: "morph"` — no `index`; label defaults to `morphKoin.label` if omitted.
- Morph entry **counts toward** `maxKnobs` (still clamped 1–3).

### 2.3 Schema version

**No bump required for Horizon 2.** `morphKoin` is an **optional additive field** on schema v3. Loaders that do not implement the morph executor **ignore** `morphKoin` at runtime but should **preserve** it through load/save (serializer stub).

Reserve **schema v4** only if a future change breaks keyframe shape (e.g. per-keyframe easing curves as first-class arrays).

---

## 3. PLAY layout vs `uiFocus.maxKnobs`

**Recommendation: morph + 2 macro KOINS = 3 controls total** (fits `maxKnobs: 3`).

| Slot | Kind | Role | Frames analog |
|------|------|------|---------------|
| **Hub / primary** | `morph` | Scene crossfade between keyframes | FRAME knob |
| **Orbit A** | `macro` index 0 | Live Spread — timbre body | Channel attenuverters at one keyframe |
| **Orbit B** | `macro` index 1 | Live Spread — motion / space | Same |

**Morph does not replace a macro slot silently** — it **uses one of the three** PLAY slots. Patches **without** `morphKoin` keep today’s behavior: up to **3 macro KOINS**.

| Patch profile | `uiFocus` layout |
|---------------|------------------|
| Pad with scene evolution | morph + Macro1 + Macro2 |
| Bass / pluck (no morph) | Macro1 (+ optional Macro2, Macro3) |
| Morph-only demo (H2 metadata) | morph + 0–2 macros; min 1 knob total |

**UI policy (Horizon 2):** Basic/Compact show morph KOIN **only when** `morphKoin` is present **and** `uiFocus` lists `kind: "morph"`. Until Horizon 3 DSP, the morph knob may bind to a placeholder APVTS param or display metadata only (mission card shows keyframe names).

Compact teleprompter: morph at **scope hub center**; Macro1/2 in **cardinal orbit** (same 3-KOIN geometry as today).

---

## 4. Agentic contract — morph vs macro KOIN

### When to use **macro KOIN** (`set_macro_koin`)

- **Live gestural spread** across 2–4 destinations from one axis (`modRoutes`).
- Performance copy: “turn BLOOM — filter + WT move together **right now**.”
- PoliMATHS Spread / Hydrasynth macro analog.
- Requires `layerA.modRoutes` with `source: macro1..macro3`.

### When to use **morph KOIN** (`set_morph_koin`)

- **Authored scene crossfade** between 2–4 **named moments** (TIGHT ↔ BLOOM ↔ VOID).
- Pad/evolving texture where the player scrubs **between presets-in-one**.
- Frames FRAME knob analog — **not** a replacement for macro routes on the same params unless documented (prefer morph on baseline, macros on deltas).
- Use when agent can describe **distinct snapshot roles**, not just “more filter.”

### Combined patch (recommended for cinematic pads)

1. `set_morph_koin` — 2–4 keyframes with `macroValues` + selective `paramOverrides` (filter, WT, reverb send).
2. `set_macro_koin` slot 0 — BLOOM spread (filter + WT motion **within** a scene).
3. `set_macro_koin` slot 1 — SPACE spread (reverb/delay/width).
4. `uiFocus`: morph + macro0 + macro1; `maxKnobs: 3`.
5. Optional: `voiceSettings.macroDissemination: true` on pads (Horizon 3: extend to morph position freeze).

### Anti-patterns

- Using macro KOINS alone to simulate morph (two presets in a bank, manual A/B) — OK for factory demos, not agent default.
- Putting morph keyframes only in `metadata.description` without `morphKoin` JSON.
- More than 4 keyframes (cap enforced by MCP + spec).
- Using `layerMorph` for multi-param performance morph — `layerMorph` remains **2-layer signal-path crossfade** (PLANNED DSP), separate from morph KOIN.

---

## 5. Horizon 3 executor outline

Runtime morph applies **after** patch load baseline, **before** or **merged with** mod-matrix macro deltas (exact order TBD in engine design):

```
1. Resolve segment: keyframes sorted by position; find i where pos[i] ≤ morphPos ≤ pos[i+1]
2. localT = (morphPos - pos[i]) / (pos[i+1] - pos[i])
3. Apply curve: linear | smoothstep | step(localT)
4. For each keyed field:
     value = lerp(keyframe[i].value, keyframe[i+1].value, localT)
5. Write macroValues[i] → macros[i].value (or APVTS macro params)
6. Write paramOverrides → target APVTS / patch fields
7. Mod matrix runs with morphed macro baselines as sources
```

**Optional extensions (Horizon 3+):**

- **Dissemination freeze:** capture `morphKoin.position` at `noteOn` per voice (extends `voiceSettings`).
- **Per-route easing** (Frames parity): `keyframes[].curvePerParam` — defer to v4 if needed.
- **FR.STEP sync:** trigger when morph position crosses a keyframe boundary.
- **Autoplay:** LFO/mod route drives morph `position` for evolving pads.

**APVTS binding:** Introduce `morphPosition` param (0..1) or reuse a dedicated macro slot — implementation detail for H3; metadata uses `morphKoin.position`.

---

## 6. Example — CATHEDRAL NEBULA-style pad

Illustrative JSON (metadata-first; based on `content/presets/factory/Interstellar/001-cathedral-nebula.pw8` character). Macro routes for BLOOM/SPACE unchanged from factory; morph adds **scene** between tight cathedral and wide nebula wash.

```jsonc
{
  "schemaVersion": 3,
  "metadata": {
    "name": "CATHEDRAL NEBULA",
    "category": "pad",
    "description": "Hold a chord — EVOLVE scrubs tight cathedral ↔ nebula bloom; BLOOM and SPACE spread within the scene."
  },
  "voiceSettings": {
    "macroDissemination": true,
    "disseminationDepth": 0.28
  },
  "macros": [
    { "id": "m1", "name": "BLOOM", "description": "Per-note spread on filter + WT.", "value": 0.55 },
    { "id": "m2", "name": "SPACE", "description": "Reverb wash + stereo width.", "value": 0.4 },
    { "id": "m3", "name": "Macro 3", "value": 0.0 }
  ],
  "morphKoin": {
    "label": "EVOLVE",
    "description": "TIGHT cathedral ↔ BLOOM hall ↔ VOID nebula.",
    "defaultPosition": 0.25,
    "position": 0.25,
    "curve": "smooth",
    "keyframes": [
      {
        "name": "TIGHT",
        "position": 0.0,
        "macroValues": [0.12, 0.05, 0, 0, 0, 0, 0, 0],
        "paramOverrides": {
          "filterCutoffHz": 750.0,
          "layerA.filter1.resonance": 0.1
        }
      },
      {
        "name": "CATHEDRAL",
        "position": 0.35,
        "macroValues": [0.45, 0.25, 0, 0, 0, 0, 0, 0],
        "paramOverrides": {
          "filterCutoffHz": 3200.0
        }
      },
      {
        "name": "NEBULA",
        "position": 1.0,
        "macroValues": [0.75, 0.65, 0, 0, 0, 0, 0, 0],
        "paramOverrides": {
          "filterCutoffHz": 12000.0,
          "masterEffects[2].mix": 0.58
        }
      }
    ]
  },
  "uiFocus": {
    "maxKnobs": 3,
    "knobs": [
      { "kind": "morph", "label": "EVOLVE" },
      { "kind": "macro", "index": 0, "label": "BLOOM" },
      { "kind": "macro", "index": 1, "label": "SPACE" }
    ]
  },
  "layerA": {
    "modRoutes": [
      { "source": 21, "destination": 1, "targetIndex": 0, "amount": 18.0, "scope": 0 },
      { "source": 21, "destination": 5, "targetIndex": 0, "amount": 0.35, "scope": 0 },
      { "source": 22, "destination": 1, "targetIndex": 0, "amount": 10.0, "scope": 0 }
    ]
  }
}
```

---

## 7. C++ model stubs

See `engine/include/pw8/patch/Patch.hpp`:

- `MorphKoinKeyframe`, `MorphKoin` structs on `Patch::morphKoin`
- `UiFocusKnobKind::Morph` for `uiFocus` entries

Serializer: round-trip `morphKoin` JSON (Horizon 2); executor **TODO Horizon 3**.

---

## 8. MCP

- **Tool:** `set_morph_koin(patch_id, label, keyframes, default_position, …)` — see `mcp_server/patch_builder.py`
- Writes `morphKoin` + `uiFocus` morph entry; optional paired macro KOINS via separate `set_macro_koin` calls.

---

## 9. References

| Doc | Path |
|-----|------|
| Frames research | `docs/MUTABLE_INSTRUMENTS_FRAMES_RESEARCH.md` |
| PoliMATHS Spread | `docs/MAKE_NOISE_POLIMATHS_RESEARCH.md` |
| Macro KOINS | `docs/ASM_MACRO_KOINS_RESEARCH.md` |
| Patch format | `docs/PATCH_FORMAT.md` |
| Example factory pad | `content/presets/factory/Interstellar/001-cathedral-nebula.pw8` |

---

## Executive summary

**Schema:** Optional top-level `morphKoin` with 2–4 keyframes (`macroValues`, `paramOverrides`), plus `uiFocus` entry `kind: "morph"`. Schema v3 additive — no version bump for metadata.

**PLAY layout:** **Morph + 2 macro KOINS** (3 total) when morph is defined; morph occupies one `maxKnobs` slot, not an extra fourth knob.

**Deferred to Horizon 3:** Morph position APVTS param, keyframe lerp executor, paramOverride path resolver, dissemination freeze of morph position, easing per segment, UI knob wiring beyond metadata/mission card.
