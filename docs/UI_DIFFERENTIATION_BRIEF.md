# MURMUR — UI Differentiation Brief

**Audience:** Curtis / design sprint steering  
**Scope:** UI refinement and identity only — no new features  
**Date:** Aug 2026  
**Codebase snapshot:** OBSIDIAN skin, `MurmurRootEditor` PLAY/DESIGN split, paged Advanced PLAY, 900 factory presets incl. Interstellar bank

---

## Executive summary

MURMUR already has **real product differentiation** in the engine (typed 8-node algorithm graph, patch-authored performance surface, wavetable warp + mesh preview, hardware-first MIDI). The UI has **strong primitives** (wireframe mesh, KOINS, mod-ring colors, Obsidian duotone) but suffers a **visual identity fracture**: the signature circular algorithm graph was built, then sidelined in PLAY for a compact chip row, while DESIGN authoring is a spreadsheet edge list with no graph preview. Competitors cannot copy the graph engine — but today the UI partially hides it.

**Strategic bet:** Re-unify around *topology you can feel while playing* and *patch-authored performance*, not more panels. Refinement, not feature sprawl.

---

## A. What makes MURMUR unique (product truth)

### Structural differentiators (engine + format)

| Truth | Why it matters | What competitors do instead |
|-------|----------------|----------------------------|
| **8-node typed algorithm graph** compiled and executed per voice | FM stacks, parallel carriers, feedback bells, ring mod — as *topology*, not per-osc warp params | **Serum:** fixed osc→filter flow, wt warps per osc. **Phase Plant:** vertical lanes, explicitly *not* a node graph. **Zebra 3:** 4-lane wireless grid, not a free typed graph. **Operator:** 4-op FM, no general graph |
| **8 engine types per node** (Classic, WT, FM, Phase, Additive, Noise, Granular, Resonator) | One instrument, many synthesis paradigms in one voice | Serum: 5 source types but no FM graph. Phase Plant: generators + Snapins. Kilohearts: effect/modular lanes, not 8-slot algorithm |
| **`.pw8` JSON patches** with `uiFocus`, macros, metadata, mod routes | Preset *tells the UI what to show*; diffable, agent-generatable | Binary blobs or opaque presets; performance macros exist elsewhere but rarely drive a dedicated PLAY surface |
| **Deterministic headless render** (`pw8-render`) | AI / Patchforge pipeline — none of the three benchmark synths target this | Host-only real-time instruments |

### Experience differentiators (already in product docs + code)

| Truth | Evidence in codebase |
|-------|---------------------|
| **PLAY = performance, DESIGN = authoring** | `MurmurRootEditor` mode toggle; PLAY never commits graph topology; DESIGN `commitAlgorithmGraph()` |
| **Knobs of Interest (KOINS)** — 6 curated controls per preset | `PatchFocusPanel` reads `uiFocus` or infers from macros/mod routes; Basic view hero layout |
| **Hardware-first performance path** | Factory CC map (MW→cutoff, EXP→res, CC74→Macro 3); MP11SE zone doc; Smart Controls template; live MW/EXP badges on KOINS |
| **Interstellar-scale preset narrative** | 100 `Interstellar/` presets with performance instructions in descriptions (register, hold time, MW behavior); golden test coverage |
| **Warp suite integrated with honest preview** | `WavetableWarp.hpp` DSP ↔ `WavetableStackView` mesh uses same phase warp; PLAY Bend+Asym, DESIGN full panel |
| **Wireframe visualization language** | Filter/LFO/FX/mod/envelope/wt mesh — procedural, no GPU theater; reads as one family |

### Tagline alignment

**"Eight engines. One voice."** / **"SOUND IN MOTION"** (header) — the UI should make *motion* visible: graph pulses, scope, KOINS responding to MW, mesh row highlight at WT pos. Static spreadsheet rows fight the brand.

### What we should NOT claim

- Not "more knobs than Serum" (parameter count ≠ identity)
- Not "better wavetable editor" (Serum/Zebra win on wt authoring depth)
- Not Eurorack cable porn (explicit non-goal in master spec and `DESIGN_AND_WARPS_PLAN.md`)

---

## B. UI differentiation opportunities (specific to this codebase)

### Current architecture (as built)

```
MurmurRootEditor
├── PatchBrowserBar (mark, MURMUR wordmark, preset, browse, scope, VOL)
├── PLAY | DESIGN toggle
├── PlayModeEditor
│   ├── View: Basic | Advanced | Compact
│   ├── Basic → PatchFocusPanel (KOINS + MW/EXP badges)
│   ├── Advanced → NodeSelectorRow + ContextStrip + tabs (OSC/FILTER/ENV/MOD/FX)
│   └── Compact → 320px column (scope, KOINS×4, preset step)
└── DesignModeEditor
    └── tabs: Graph | Matrix | FX | Wavetable
        ├── AlgorithmGraphEditor (edge list + combos)
        ├── ModMatrixDesignPanel
        ├── DesignFxDetailPanel
        └── WavetableWarpPanel + stack mesh
```

**Shared chrome:** `SharedEditorChrome` = preset bar + favorites + overlay only — mode toggle lives in root, not a unified "Backstrom chrome system" yet.

### Visual hierarchy pain points

1. **Signature graph orphaned** — `AlgorithmGraphView` (circular, pulsing edges, caption sentence, edge legend) is implemented but **not mounted in PLAY** after `NodeSelectorRow` replaced it (`UI_PAGED_LAYOUT.md` recommendation landed). PLAY Advanced loses the #1 visual differentiator.
2. **DESIGN graph is a spreadsheet** — `AlgorithmGraphEditor` is combo boxes and edge rows; includes `AlgorithmGraphView.h` but no preview pane wired. Authoring feels like admin UI, not "algorithm instrument."
3. **Tri-accent drift** — Palette defines cyan (structural), amber (performance), violet (brand/knob orbit). Docs say duotone; UI adds violet glow + brand gradient in header. Risk of "neon soup" if every layer adds a glow pass.
4. **Tab + view-mode stacking** — Root: PLAY/DESIGN. PLAY: Basic/Advanced/Compact. Advanced: OSC/FILTER/ENV/MOD/FX. DESIGN: 4 tabs. **Five navigation dimensions** before touching a knob.
5. **Redundant node selection** — `NodeSelectorRow` in PLAY Advanced, again inside `WavetableWarpPanel`, similar engine pills in `OperatorEditorPanel`. Same mental model, three visual dialects.
6. **Sprite/icon gap** — Only asset: `murmur_mark_512.png`. Engines = 3-letter text (CLS/WT/FM). Edge types = color lines (in unused graph view). Mod sources = colored chips (good). **No icon system** for engines, edge types, FX algorithms, categories.
7. **KOINS vs buried params** — Basic view is correct strategy; Advanced OSC still exposes full operator grid. Risk: users live in Advanced and never feel patch curation.
8. **Compact mode under-branded** — 320px column is MP11SE/Logic Smart Control companion — should feel like a **instrument teleprompter**, not a shrunken main UI.
9. **Interstellar narrative invisible in chrome** — Rich preset descriptions exist; UI shows name pill only. Missed emotional differentiation vs generic preset browsers.
10. **Accessibility debt** — Hand-painted pills/chips/graph nodes (documented in `UI.md`); refinement sprint should not add more manual hit-targets without a plan.

### Views to *redesign* (not add)

| View | Refinement direction |
|------|---------------------|
| **PLAY Basic** | KOINS as 80% of viewport; preset performance hint line from metadata; MW/EXP badges larger; optional *collapsed topology strip* (see signature moment) |
| **PLAY Advanced** | Restore topology read — mini graph OR edge strip between `NodeSelectorRow` and ContextStrip; unify node chip visual with DESIGN |
| **PLAY Compact** | Circular scope + KOINS only; typographic preset "mission card"; hide view-mode chrome noise |
| **DESIGN Graph tab** | Split pane: **live `AlgorithmGraphView` preview** (top/left) + edge list (bottom/right); Apply/Revert anchored to preview |
| **DESIGN Matrix tab** | Reuse mod chip colors + wireframe routing preview (`ModRoutingWireframeView` already exists in PLAY mod strip) |
| **PatchBrowserBar** | Category/mood glyph from metadata; Interstellar patches get subtle HUD treatment; scope mode toggle less prominent in Basic |
| **OperatorEditorPanel** | Engine pills → icon+label; wt mesh gets consistent panel frame with filter wireframe |
| **PresetBrowserOverlay** | Tag chips, performance hint subline from description (first sentence) |

### Sprite / icon system opportunities

**No sprite sheet exists today.** Recommendation: one **SVG-derived raster atlas** (or small set of `@2x` PNGs) generated from a single design pass:

| Family | Count | Usage |
|--------|-------|-------|
| Engine type | 8 | Node chips, graph nodes, operator pills |
| Edge type | 7 | Graph edges, legend, DESIGN edge rows |
| Mod source category | 6 | LFO, ENV, MIDI, Macro, Velocity, MPE — chips reuse palette colors |
| FX algorithm | ~12 | FxChainStrip type labels |
| Scope mode | 3 | Spectrum / waveform / off |
| Category mood | 6 | Bass, Lead, Pad, Seq, Ambient, Interstellar |

**Style:** Monoline, 16×16 / 24×24, single-stroke, matches wireframe mesh — not skeuomorphic knobs. Cyan stroke default; amber for performance-touched icons; violet only for brand mark.

**Implementation note (future):** `BrandingAssets` becomes `IconAtlas` alongside mark; keep `ObsidianPalette` as single color source.

### Information architecture: declutter PLAY/DESIGN

| Keep in PLAY only | Keep in DESIGN only | Shared chrome |
|-------------------|---------------------|---------------|
| KOINS, MW/EXP badges, scope, arp launcher | Edge edit, full matrix grid, FX field specs, wt warp full panel | Preset bar, favorites, load/save, master vol |
| Node select + operator *performance* params | Graph Apply/Revert, Open Builder handoff | Mode toggle |
| Mod overlay (gesture UX) | Matrix batch commit | |
| Read-only topology visualization | Wavetable assign + warp authoring | |

**Reduce redundancy:** One `NodeSelector` component shared across PLAY Advanced, DESIGN wt panel, and operator panel — same icons, same output-node dimming, same unimplemented dashed state.

### Graph visualization vs edge-list — close the gap

| Surface | Today | Target |
|---------|-------|--------|
| PLAY | Chips only; graph code dormant | Collapsed topology: chips + 1-line caption + "expand graph" overlay using existing `AlgorithmGraphView` |
| DESIGN | Edge spreadsheet | **List edits graph; graph previews list** — single source of truth `workingCopy_` drives both |

This uses existing code; it is re-wiring and layout, not new DSP.

---

## C. Challenge prompts — "Backstrom UI"

### Five design principles

1. **Performance is the product.** If a control doesn't serve playing, recording, or browsing sound in Logic, demote it behind DESIGN or Advanced.
2. **Topology is the logo.** The algorithm graph is MURMUR's Ableton-style session view — not decoration. Never ship a mode where the graph is harder to find than a combo box.
3. **The patch speaks first.** KOINS, macro names, metadata hints, MW routes — UI copy comes from `.pw8`, not generic labels. Factory presets already encode performance instructions; surface them.
4. **One wireframe language.** Wavetable mesh, filter curve, LFO line, FX flow, envelope path — same projection, glow, and stroke weight. If it moves or shapes sound, it looks like it belongs to the same instrument.
5. **Mode honesty.** PLAY feels like a stage; DESIGN feels like a lab. Same palette, different density — never identical tab sets with different names.

### Anti-patterns to kill

| Anti-pattern | MURMUR instance | Kill move |
|--------------|-----------------|-----------|
| Checkbox feature UI | Output toggles as raw checkboxes in DESIGN graph | Custom node tiles with output badge |
| Tab soup | PLAY view modes × Advanced pages × DESIGN tabs | Basic default; Advanced remembers last page; DESIGN ≤4 tabs with icons |
| Spreadsheet synth | DESIGN edge rows without graph | Split preview (above) |
| Parameter wall | Full APVTS exposure fear | KOINS + "designer pages" — resist adding 6th PLAY tab |
| Neon soup | Violet header glow + cyan graph + amber macros + mod rainbow | Document strict roles: violet = brand/motion on controls only; cyan = signal structure; amber = human touch |
| Fake depth | Abandoned AI art backgrounds (`UI.md` GATE 6) | Procedural wireframe only — honesty as aesthetic |
| Generic JUCE layout | Radio groups of `TextButton` tabs everywhere | SectionPanel headers, icon tabs, consistent radii |
| Duplicate navigation | Three node selectors | One component |
| Dead differentiator | `AlgorithmGraphView` unused | Mount or delete — don't leave in limbo |

### Reference boards (sensibility, not copying)

| Reference | Steal sensibility | Do NOT steal |
|-----------|-------------------|--------------|
| **Norns** | Script/patch metadata drives UI; sparse, purposeful | Grid controller layout |
| **Ableton Device** | Macro row, preset name dominance, flat hierarchy | Ableton colors/layout |
| **Teenage Engineering** | Typographic restraint, one accent, hardware labels | Toy form factor |
| **Interstellar film UI** | Mission labels, coordinate readouts, sparse HUD overlays | Literal NASA clip art |
| **u-he Zebra** | Wireless mod assignment — no cables | Panel dimensions / fonts |
| **Mutable Instruments** | Engine mode icons, honest "this mode is digital" | Hardware panel photos |
| **Serum** | Mod ring color traceability (already in palette) | Wavetable editor chrome |
| **Monome Arc** | Circular metaphors for cyclic data | Hardware |

---

## D. Concrete refinement roadmap (6 weeks, UI-only)

### Week 1 — Identity audit & icon spec ✅ (Aug 2026)

**Focus:** Naming, color roles, icon grid on paper/Figma  
**Deliverables:**
- ✅ Color role doc: cyan / amber / violet usage rules (update `ObsidianPalette.h` comments)
- ✅ 8+7 icon grid approved — engine icons implemented as JUCE paths (`EngineIconGrid`); edge icons Week 2
- ⏳ KOINS Basic layout mock — knob sizing, badge placement, metadata hint line (hint line shipped in bar; KOINS mock Week 2)
- ✅ DESIGN Graph tab: live `AlgorithmGraphView` preview wired above edge list (`AlgorithmGraphEditor`)
- ✅ Preset description first sentence under preset name in `PatchBrowserBar`
- ✅ Engine icon paths wired into `NodeSelectorRow` pills (text fallback retained)
**Quick wins:** Preset description first sentence under preset name in bar; unify "MURMUR" vs stale "Patchwork Eight" in any remaining docs  
**Deep cut:** None — research week  
**Logic prototype:** Screenshot Basic view with annotation overlay for Ben

### Week 2 — Graph reunification

**Focus:** Bring topology back to PLAY; preview in DESIGN  
**Deliverables:**
- PLAY Advanced: graph overlay toggle OR thin topology caption strip under `NodeSelectorRow`
- DESIGN Graph tab: split `AlgorithmGraphView` preview + existing edge editor
- Shared node tile component (chip = mini graph node)  
**Quick wins:** Wire existing `AlgorithmGraphView` into DESIGN (code exists)  
**Deep cut:** PLAY overlay interaction polish (click node in graph = select chip)  
**Logic prototype:** Load Interstellar preset, open graph overlay, verify legibility at 1280×720

### Week 3 — Information architecture pass

**Focus:** Reduce navigation dimensions  
**Deliverables:**
- Default launch = Basic (verify)
- Collapse view-mode row visual weight — Compact as icon, not third equal tab
- DESIGN tab icons + labels; Matrix tab embeds wireframe routing preview
- Deduplicate `NodeSelectorRow` → shared `EngineNodeStrip`  
**Quick wins:** Hide DESIGN button until user holds Option or explicit "author" entry — *optional*, test with Ben  
**Deep cut:** Re-scope Advanced tabs → merge ENV into OSC or FILTER where overlap exists  
**Logic prototype:** Timed task — "find cutoff and add LFO route" in <30s from cold open

### Week 4 — Wireframe & panel consistency

**Focus:** One visual family across previews  
**Deliverables:**
- Shared `WireframePanel` frame (title dot, corner radius, glow stroke) on filter/LFO/FX/wt mesh
- `OperatorEditorPanel` layout rhythm — wt mesh height budget without starving knobs
- Mod chip icons from atlas  
**Quick wins:** Filter + wt mesh same panel header treatment  
**Deep cut:** FxChainStrip wireframe + strip alignment pass  
**Logic prototype:** A/B wt mesh + filter panel while turning KOINS

### Week 5 — Performance & narrative layer

**Focus:** MP11SE / Smart Controls / Interstellar identity  
**Deliverables:**
- Compact mode "mission card" — preset name, category, one-line play hint
- Interstellar category subtle HUD frame in browser + bar (not gimmicky — thin coordinate tick marks)
- KOINS MW/EXP badges animate on MIDI receive
- Arp launcher chip visual aligned with icon system  
**Quick wins:** MIDI activity flash on badges  
**Deep cut:** Preset browser sort/filter by "performance ready" (has uiFocus ≥4)  
**Logic prototype:** MP11SE or Smart Controls mapping session with Ben checklist (`WEEK7_DAW_SOAK_CHECKLIST.md`)

### Week 6 — Polish, accessibility, soak

**Focus:** Ship-quality pass  
**Deliverables:**
- Tooltip pass on all icon-only controls
- Minimum accessibility: engine pills as real `ToggleButton`s OR AccessibilityHandler batch
- `pluginval` + Logic soak; screenshot pack for marketing  
**Quick wins:** Keyboard focus on preset prev/next  
**Deep cut:** VoiceOver labels for graph nodes  
**Logic prototype:** Full Ben sign-off; Curtis golden screenshot set

### Quick wins vs deep cuts summary

| Quick wins (week 1–2) | Deep cuts (week 3–6) |
|-------------------------|----------------------|
| ✅ Metadata hint under preset name | Shared node strip refactor |
| ✅ DESIGN graph preview pane | IA merge ENV/FILTER |
| Mount dormant `AlgorithmGraphView` in PLAY (Week 2) | Icon atlas pipeline in CMake |
| ✅ Color role documentation | Accessibility component refactor |
| Interstellar browser badge (Week 5) | Compact teleprompter layout |

---

## E. One bold signature moment

### **"Live Topology" — the graph breathes with your performance**

**Interaction:** In PLAY **Basic** view, the preset name / KOINS header includes a **compact topology strip** (8 dots + thin edges). When you move a KOINS knob, mod wheel, or macro:

- Edges **from** that parameter's mod sources **pulse** in edge-type color
- Output nodes **glow** in execution order (compiler order, already available)
- Holding a note animates structural pulse along active edges (existing timer logic from `AlgorithmGraphView`)

Tap the strip → **full circular graph overlay** (existing component, fullscreen dimmed) — rotate nothing, drag nothing — *watch how this patch is wired while you play*.

**Why this is MURMUR:** No competitor shows a **compiled FM/PM graph** reacting to **your** macro routes in the performance view. Serum shows wt; Phase Plant shows lanes; we show **the algorithm listening back**.

**Why it's refinement-only:** Reuses `AlgorithmGraphView`, `ModMatrixExecutor` route data, KOINS focus mapping — no new parameters.

**Sound bite for users:** *"You can see the synth thinking."*

---

## Appendix: competitor "don't do" checklist

| They do | We don't |
|---------|----------|
| Serum wt editor as hero | Our hero = algorithm + KOINS |
| Phase Plant Snapin grid | Our FX = layer inserts, not infinite modules |
| Operator 4-op diagram | We have 8 ops — show it |
| Vital rainbow skin customization | One amazing skin (OBSIDIAN) |
| Cable mod matrix | Wireless assign + optional graph |
| Generic preset name only | Performance instructions in metadata |

---

## Appendix: key file index for implementers

| Concern | Path |
|---------|------|
| Root shell | `plugin/src/ui/MurmurRootEditor.{h,cpp}` |
| PLAY layout | `plugin/src/ui/PlayModeEditor.{h,cpp}`, `PlayModeLayout.h` |
| DESIGN layout | `plugin/src/ui/DesignModeEditor.{h,cpp}` |
| KOINS | `plugin/src/ui/components/PatchFocusPanel.{h,cpp}` |
| Graph (dormant hero) | `plugin/src/ui/components/AlgorithmGraphView.{h,cpp}` |
| Graph (DESIGN list) | `plugin/src/ui/components/AlgorithmGraphEditor.{h,cpp}` |
| Node chips | `plugin/src/ui/components/NodeSelectorRow.{h,cpp}` |
| Wt mesh | `plugin/src/ui/components/WavetableStackView.{h,cpp}` |
| Brand mark | `plugin/src/ui/theme/BrandingAssets.{h,cpp}`, `plugin/resources/branding/murmur_mark_512.png` |
| Colors | `plugin/src/ui/theme/ObsidianPalette.h` |
| Product truth | `docs/product/OVERVIEW.md`, `PLAY_MODE.md`, `SOUND_DESIGN.md` |
| Plan context | `docs/DESIGN_AND_WARPS_PLAN.md`, `docs/UI.md`, `docs/UI_PAGED_LAYOUT.md` |

---

*This brief is steering documentation only. Implementation tickets should reference specific weeks above and respect the no-new-features constraint.*
