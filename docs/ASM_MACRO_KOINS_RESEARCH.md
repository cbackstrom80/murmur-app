# ASM Performance Macros → MURMUR Feature Macro KOINS

**Date:** 2026-08-13 (updated)  
**Author:** Agent research pass for Curtis  
**Goal:** Understand how Ashun Sound Machines (ASM) Hydrasynth implements Performance Macros, audit MURMUR/MURMUR KOINS today, and define **1–3 feature macro KOINS** for Basic/Compact PLAY + agentic preset generation.

---

## Feature macro KOINS policy (2026-08-13)

**KOINS = contextual macro/mod-matrix performance controls**, not generic APVTS param knobs.

| Property | Value |
|----------|-------|
| Count | **1–3** per patch in Basic/Compact PLAY |
| Engine binding | `Macro1`–`Macro8` APVTS params + `layerA.modRoutes` |
| Typical indices | 0, 1, 2 (Macro1–3) with patch-specific names |
| Visual | Featured decked knobs — warm amber accent + subtle outer-deck tint (`featuredKoin` in `DeckedKnobDraw`) |
| MW/EXP | Separate MIDI badges — not KOINS |
| Advanced PLAY | KOINS hidden; full MOD tab for power users |
| Schema | `uiFocus.maxKnobs` default **3**, clamp **1–3**; `kind: macro` only on performance surface |
| Inference | Routed macros only — **no param-kind padding** |

Agentic preset generation assigns meaningful macro names + 2–4 mod routes per featured macro reflecting patch character (e.g. CATHEDRAL NEBULA: BLOOM + SPACE routing to reverb/warp/filter).

---

## 1. ASM Hydrasynth Performance Macros (verified)

**Product:** Ashun Sound Machines Hydrasynth (Keyboard, Desktop, Explorer — same macro model across variants).

**References:**

| Source | URL |
|--------|-----|
| Hydrasynth Keyboard / Desktop Owner's Manual v2.0.5 (primary) | https://www.mecldata.com/download/asm/Hydrasynth_KB_DR_Owners_Manual_2.0.5.pdf |
| ASM product page (macros + mod matrix summary) | https://www.ashunsoundmachines.com/hydrasynth-key |
| Mod destination list (manual mirror) | https://www.manualsdir.com/manuals/857924/asm-hydrasynth-explorer-portable-digital-wave-morphing-synthesizer-keyboard-8-voice.html?page=76 |

### What the player sees

On the **Home page** (“Macro City”), eight **Control knob + Control button** pairs become live performance controls. Each pair is one **Macro**. Turning the knob sweeps assigned parameters; the button can toggle, trigger, switch, or reset (system-configurable).

- Active macros show a numeric readout on Home; empty macros show a dash.
- Macro names (up to 8 characters, preset list or custom) appear on Home and in the assign UI.
- **INIT + turn knob** jumps macro to 0 without passing through intermediate values (performance safety).

### Architecture: one knob → many destinations

Each macro is a **private mini mod matrix**, not a single parameter binding:

| Property | Hydrasynth behavior |
|----------|---------------------|
| Macros per patch | **8** |
| Destinations per macro | **Up to 8** (Des 1–8), paged 3 at a time in the assign UI |
| Per-destination depth | **Signed** (positive increases, negative decreases); manual uses ±128-style range |
| Per-destination button value | Separate **Button Value** field (knob sweeps vs button jump/toggle) |
| Destination types | Nearly any engine parameter, FX, arp settings, **mod matrix route depths** (`ModMtrx[1–32]`), MIDI CC out, CV outs |
| Reverse routing | Mod matrix sources can target **macro values**; macros can target **mod route depths** |
| Separate mod matrix | **32** general slots in addition to macro bundles |
| Curves | No per-destination curve menu in the macro chapter — depth is linear unless the underlying parameter has its own scaling |
| Save behavior | When saving a patch: macro knob positions can **Return to zero**, **Save as-is**, or **Convert** into underlying parameter values |

### Assignment UX (summary)

1. `[MACRO ASSIGN]` → pick macro slot (Control button 1–8).
2. `[INIT]` + button clears a macro.
3. **Assign** → module buttons (OSC, Filter, FX, ModMtrx, …) pick destination group + parameter.
4. Control knobs set **Depth** (amount) and **Button Value**.
5. Audition on assign page with knob 5 / button 5.
6. **Macro Slot Copy** duplicates destination group/parameter to sibling slots (same page).
7. Page 4: name from preset list (100+ names: WARMTH, SPACE, Morph, etc.) or custom entry.

### Bipolar / unipolar

- Macro depths are **bipolar** — one macro can push some destinations up and others down simultaneously.
- Mod matrix documentation notes per-voice sources can be bipolar or unipolar; macro control of a mod route depth interacts with that route’s own depth (manual warns about “dead zones” when route depth is 0 vs −128).

### Morph / performance behavior

Hydrasynth does **not** expose a global “morph macro” in the macro chapter. Performance morphing is achieved by:

- stacking several destinations under one macro knob,
- optionally modulating mod-route depths (meta-modulation),
- button modes (Toggle / Trigger / Switch / Reset) for stepped performance.

Layer/algorithm morph on Hydrasynth is a separate engine feature, not the macro system itself.

### Contrast: Serum & Vital (useful, not identical)

| | **Hydrasynth** | **Serum / Serum 2** | **Vital** |
|---|----------------|---------------------|-----------|
| Count | 8 knob+button pairs | 4 macro knobs (Serum 2 adds richer matrix linking) | 4 macro knobs |
| Multi-dest | Up to 8 dest **per macro**, dedicated assign UI | Drag macro → param; Matrix for fine depth; S2 can mod **mod amounts** | Drag macro → param or mod amount |
| Curves | Linear depth | Per-destination curve (linear, exp, log, S) | Matrix depth + flexible “mod the mod” |
| Meta-mod | Macro → ModMtrx depth | Macro → modulation depth (S2) | Macro → any mod amount |
| UX model | Hardware Home page performance | Visual drag + arc | Visual drag + matrix |

**Takeaway for MURMUR:** ASM macros are **named performance bundles** with fixed slot count, live-first UX, and optional meta-mod via mod-route depth. Serum/Vital prove the *pattern* (one knob, N destinations, signed depths) but Hydrasynth’s **8-slot / 8-dest-per-slot** layout and Home-page exclusivity match the hardware-performance goal better than Vital’s open-ended matrix.

---

## 2. MURMUR KOINS today (codebase audit)

### What KOINS are

**KOINS = Knobs of Interest** — patch-authored **performance curation**, not a separate DSP primitive.

- UI: `PatchFocusPanel` (`plugin/src/ui/components/PatchFocusPanel.{h,cpp}`)
- Logic: `inferPatchFocusKnobs()` in `plugin/src/ui/components/ModRoutingUi.cpp`
- Schema: `Patch::uiFocus` in `engine/include/pw8/patch/Patch.hpp`, documented in `docs/PATCH_FORMAT.md`

Each KOIN is either:

1. **`kind: macro`** — binds to APVTS `macro1`…`macro8` (0..1), display name from `macros[i].name` or label override, or  
2. **`kind: param`** — binds directly to a single APVTS parameter (`filterCutoffHz`, `layerPan`, …).

KOINS are **UI labels for existing parameters**. Multi-destination behavior only appears when the underlying **macro is wired in `modRoutes`** as `ModSource::Macro1`…`Macro8`.

### How many exist today

| Context | Count | Code / policy |
|---------|-------|----------------|
| Engine macros | **8** always (`Patch::macros[8]`) | `Patch.hpp`, APVTS `macro1`–`macro8` |
| Standard PLAY KOINS | **Target 6**, **minimum 4** | `kStandardKoinCount`, `kMinimumKoinCount` in `ModRoutingUi.h`; factory script `KOINS_TARGET = 6` |
| Compact PLAY KOINS | **Up to 4** (cardinal orbit) | `PatchFocusPanel`: `maxKnobs = 4` when `compactLayout_` |
| Authored list cap | `uiFocus.maxKnobs` (default 6, clamp 1–8) | `PATCH_FORMAT.md` |

**Typical factory `uiFocus` mix** (example: `content/presets/factory/Pads/07-cloud-glow.murmur`):

- 1 named macro (e.g. WARMTH → Macro 1, routed to WT position)
- 5 direct params (Cutoff, Reso, Layer, Pan, …)

So today KOINS are **mostly direct parameter knobs**, with **one macro** as the “smart control” hook — not two dedicated ASM-style macro bundles.

### What they control

**Macros (engine):**

- Stored in `.pw8` `macros[]`: `{ id, name, description, value }` (value 0..1).
- Routed via **`layerA.modRoutes`** with source ordinals **21–28** (`Macro1`–`Macro8` in JSON).
- Executed in `ModMatrixExecutor` — same path as LFO/ENV/MW routes.
- Factory layout: category primary macro → filter cutoff or WT position (`docs/MIDI_CONTROLLERS.md`).

**KOINS (UI):**

- Read `uiFocus.knobs` if non-empty; else infer:
  1. Macros that have ≥1 active mod route,
  2. APVTS params implied by active mod routes (cutoff, reso, pan, WT pos, …),
  3. Pad list: cutoff, reso, layer gain, pan, master, LFO rate,
  4. Any remaining named macros.

**Not KOINS but related performance surface:**

- Mod Wheel (CC1) → usually filter cutoff (badge in `PatchFocusPanel`)
- Expression (CC11) → resonance or op level (badge)
- MP11SE CC map mirrors CCs into Macro 2–7 (`PerformanceMidiMap.hpp`)
- `MacroStrip` (8 macro row) **exists in codebase but is not mounted** in `PlayModeEditor` — KOINS replaced visible macro row in PLAY

### Basic vs Compact vs Advanced exposure

**Product direction (2026-08-13):** PLAY-only UI — **no Design Mode**. Live topology **removed** from Basic/Compact. Advanced screen **stays** (OSC/FILTER/ENV/MOD/FX tabs + mod routing overlay).

| Surface | Basic | Compact (320px teleprompter) | Advanced |
|---------|-------|------------------------------|----------|
| KOINS / PatchFocusPanel | **Target: 2 macro knobs** (today: 6 knobs) | **Target: 2 macro knobs** orbiting scope + mission card (today: 4) | **Hidden** |
| Live topology | **Removed** | **Removed** | **Removed** (no live graph strip) |
| Mod Matrix UI | Hidden (overlay closed) | Hidden | MOD tab + `ModRoutingOverlay` (M key) |
| Macro strip (8) | Not shown | Not shown | Not shown |
| OSC/FILTER/ENV/MOD/FX tabs | Hidden | Hidden | Shown |
| MW/EXP badges | On PatchFocusPanel | Not in compact orbit layout | N/A |

View wiring: `PlayModeEditor::setViewMode()` — Basic shows `patchFocusPanel_` only; Compact delegates to `CompactModeEditor` + `focusPanel_.setCompactLayout(true)`.

### Patch / mod routing that can back 2 macro-KOINS

**Already implemented (no new DSP needed):**

```
macroN.value (0..1)  →  ModSource::MacroN  →  modRoutes[]  →  destinations
```

- Up to **64** mod routes per layer (`kMaxModRoutes`).
- Destinations today: filter cutoff/reso (global + per-op), op level, pan, WT position + warp fields (`docs/MODULATION.md`).
- Multiple routes per macro = ASM-style multi-destination (different `amount` per route).
- Signed amounts supported (e.g. −72..+72 st on cutoff).

**Not implemented (ASM parity gaps):**

- No first-class `macroDestinations[]` on `Patch::Macro` — routing is flat mod matrix only.
- **Meta-modulation** (macro → mod route depth) — explicitly PLANNED, not in executor.
- No per-destination curves on macro routes.
- Macro button / toggle semantics (Hydrasynth button modes) — N/A on MURMUR UI.

**Agentic preset generation today:**

| Path | Macro / KOINS support |
|------|------------------------|
| `scripts/generate_factory_presets.py` | Names 8 macros; `infer_ui_focus()` builds 6-entry `uiFocus` (macros + params) |
| `mcp_server/patch_builder.py` | `add_mod_route(source="macro1", …)`; init patch has 8 unrouted macros |
| `mcp_server/` | **No** `set_ui_focus`, **no** `set_macro_bundle`, **no** “active macro only” filter |
| `docs/MCP_AND_NL_PATCH_GENERATION.md` | Notes Python API partial; JSON schema is source of truth |

---

## 3. Recommended architecture: exactly 2 macro-KOINS

### Design principle

> **Two visible performance knobs. Everything else is patch-internal or Advanced-only.**

Align KOINS with ASM **macro bundles**, not a scatter of raw APVTS params. Mod wheel, expression, and hardware CCs remain separate “expressive MIDI” channels — not KOINS.

### Slot assignment

| KOIN slot | Engine macro | Role | Suggested default names (agent fills per patch) |
|-----------|--------------|------|--------------------------------------------------|
| **KOIN A** | `Macro1` (index 0) | Primary timbre / body | WARMTH, PUNCH, EDGE, BODY |
| **KOIN B** | `Macro2` (index 1) | Motion / space / brightness | MOTION, SPACE, BLOOM, AIR |

Reserve `Macro3`–`Macro8` for:

- MP11SE CC mirroring (already mapped),
- host Smart Controls,
- Advanced/automation lanes,

—not Basic/Compact KOINS.

### Data model (minimal — reuse mod matrix)

**Phase 1 (MVP): no schema change.** A macro-KOIN is:

```jsonc
// uiFocus — exactly 2 macro entries
"uiFocus": {
  "maxKnobs": 2,
  "knobs": [
    { "kind": "macro", "index": 0, "label": "WARMTH" },
    { "kind": "macro", "index": 1, "label": "SPACE" }
  ]
}

// modRoutes — N rows per macro, only rows with |amount| > epsilon
{ "source": 21, "destination": 5, "targetIndex": 0, "amount": 0.35, "scope": 0 },
{ "source": 21, "destination": 1, "targetIndex": 0, "amount": 18.0, "scope": 0 },
{ "source": 22, "destination": 1, "targetIndex": 0, "amount": 12.0, "scope": 0 }
```

**Phase 2 (optional polish):** add compact authoring block (agent-friendly):

```jsonc
"macroBundles": [
  {
    "macroIndex": 0,
    "label": "WARMTH",
    "destinations": [
      { "destination": "filter_cutoff", "targetIndex": 0, "amount": 18.0, "scope": "voice" },
      { "destination": "op_wt_position", "targetIndex": 0, "amount": 0.35, "scope": "voice" }
    ]
  },
  {
    "macroIndex": 1,
    "label": "SPACE",
    "destinations": [
      { "destination": "filter_cutoff", "targetIndex": 0, "amount": 10.0, "scope": "voice" }
    ]
  }
]
```

Serializer expands `macroBundles` → `modRoutes` on load (single source of truth for agents). Human Advanced UI can edit routes directly until a macro assign screen exists.

### UI changes (conceptual)

1. **`kStandardKoinCount = 2`**, **`kMinimumKoinCount = 2`**, Compact **`maxKnobs = 2`** (orbit N/E or W/S).
2. **`inferPatchFocusKnobs()`**: if `uiFocus` empty, pick first two macros with active routes; **do not pad with raw cutoff/reso params** in Basic/Compact.
3. **Remove param-kind KOINS** from factory generator default (`infer_ui_focus` rewrite).
4. Keep MW/EXP badges on Basic; optionally single-line hint (“MW → Cutoff”) without extra knobs.
5. Advanced: unchanged MOD tab / overlay; all 8 macros still in APVTS for DAW.

### Agentic preset schema / tool changes

**Authoring contract for agents (JSON fields):**

| Field | Required | Notes |
|-------|----------|-------|
| `macros[0].name`, `macros[1].name` | Yes | Performance labels (≤8 chars ASM style) |
| `macros[0..1].description` | Recommended | One-line player hint for mission card |
| `uiFocus.maxKnobs` | Yes | **2** |
| `uiFocus.knobs` | Yes | Exactly 2 macro entries, indices 0 and 1 |
| `layerA.modRoutes` | Yes | ≥1 route per used macro; omit unused Macro3–8 routes |
| `metadata.description` | Recommended | First sentence = Compact teleprompter hint |

**MCP tools to add:**

- `set_macro_koin(patch_id, slot: 0|1, name, destinations[])` — writes macro name + routes + uiFocus slot.
- `set_ui_focus(patch_id, knobs[])` — validate exactly 2 macro knobs.
- `list_active_macros(patch_id)` — returns only macros with routes (agent introspection).

**Factory generator (`generate_factory_presets.py`):**

- `KOINS_TARGET = 2`, `KOINS_MINIMUM = 2`.
- `infer_ui_focus()` → macro 0 + macro 1 only; wire category-appropriate multi-dest bundles (2–4 routes each max for agent compactness).

**Validation rule (agent + CI):**

- Basic/Compact patches: `uiFocus.knobs.length == 2`, all `kind == macro`, indices ⊆ {0,1}.
- Each referenced macro must have ≥1 active `modRoutes` entry.
- Warn if Macro3–8 have routes but are not in uiFocus (Advanced-only macros — OK if intentional).

---

## 4. Basic vs Compact exposure table (target state)

| Control | Basic PLAY | Compact PLAY | Advanced PLAY | Hidden from agent Basic preset |
|---------|------------|--------------|---------------|--------------------------------|
| KOIN A (Macro 1) | Large knob | Orbit knob | — | — |
| KOIN B (Macro 2) | Large knob | Orbit knob | — | — |
| Mod Wheel | Badge | (implicit in patch) | Badge if KOINS panel shown | Route only, no extra knob |
| Expression | Badge | — | — | Route only |
| Live topology strip | **Removed** | **Removed** | **Removed** | — |
| Cutoff / Reso direct | **Remove from KOINS** | **Remove** | FILTER tab | Yes — use macro routes instead |
| Macro 3–8 | APVTS only | APVTS only | MOD / DAW | Omit routes unless needed |
| Full mod matrix | Hidden | Hidden | MOD tab + overlay | Author routes, don’t expose UI |

---

## 5. Implementation phases

### Phase 0 — Documentation & generator (this doc)

- [x] Research ASM + audit KOINS
- [ ] Align factory script + showcase presets to 2 macro-KOINS (bulk pass)
- [ ] Update `docs/product/PLAY_MODE.md`, `MIDI_CONTROLLERS.md` KOINS policy

### Phase 1 — MVP (minimal code)

1. Change `kStandardKoinCount` / `kMinimumKoinCount` to **2**.
2. Compact orbit: 2 knobs (adjust `PatchFocusPanel` angles / layout).
3. Tighten `inferPatchFocusKnobs()` / `padPatchFocusKnobs()` — macro-only, max 2, no param padding in performance views.
4. Update `generate_factory_presets.infer_ui_focus()` for 2 macro slots.
5. MCP: `set_macro_koin` helper wrapping `add_mod_route` + uiFocus write.

**Acceptance:** Load factory pad in Basic → exactly **2 named macro knobs**; turning each affects multiple parameters via existing mod matrix.

### Phase 2 — Agent polish

1. Optional `macroBundles` schema + serializer expansion.
2. MCP validation tool + preset linter in CI.
3. ✅ Mission card copy from `macros[i].description` (“WARMTH: opens filter + wavetable”) — `performanceHintForPatch()` in ModRoutingUi; PatchBrowserBar, CompactModeEditor, PatchFocusPanel.
4. ✅ MCP `set_macro_koin` helper (`mcp_server/patch_builder.py`, `server.py`).

### Phase 3 — ASM parity (defer)

- Macro assign UI (Hydrasynth-style destination picker).
- Meta-mod: macro → mod route depth.
- Per-route curves.
- Macro button semantics (if hardware controller adds buttons).

---

## 6. Curtis action checklist

1. **Approve the 2-KOIN policy** — Macro1 + Macro2 only in Basic/Compact; drop param-kind KOINS from performance surface.
2. **Pick naming convention** — patch-specific names (WARMTH/SPACE) vs fixed global labels; docs assume patch-authored (ASM-aligned).
3. **Scope agent PR** — factory regen + `infer_ui_focus` + constants first; UI constant changes second; defer `macroBundles` schema unless agents need it immediately.
4. **Leave MP11SE CC → Macro3–7 map intact** — hardware continues to work; those macros simply won’t appear as KOINS.
5. **Advanced PLAY unchanged** — full MOD tab remains the escape hatch for power users. **No Design Mode** — patch authoring is agentic/offline; PLAY exposes only what each preset needs.

---

## 7. Key file index

| Topic | Path |
|-------|------|
| KOINS UI | `plugin/src/ui/components/PatchFocusPanel.{h,cpp}` |
| KOINS inference | `plugin/src/ui/components/ModRoutingUi.{h,cpp}` |
| PLAY view modes | `plugin/src/ui/PlayModeEditor.{h,cpp}` |
| Compact teleprompter | `plugin/src/ui/components/CompactModeEditor.{h,cpp}` |
| Patch schema | `engine/include/pw8/patch/Patch.hpp`, `docs/PATCH_FORMAT.md` |
| Mod execution | `engine/include/pw8/modulation/ModMatrixExecutor.hpp` |
| Factory KOINS | `scripts/generate_factory_presets.py` (`infer_ui_focus`) |
| Agent MCP | `mcp_server/patch_builder.py`, `mcp_server/patch_schema.py` |
| MIDI / macro policy | `docs/MIDI_CONTROLLERS.md` |
