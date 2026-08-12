# PLAY Mode: Compact Graph Row + Paged Layout

**PLAN, not built.** Captured per explicit direction: the algorithm graph
eats too much vertical space, and PLAY mode should restructure around a
compact "Basic" performance view plus separate pages (OSC/FILTER/MOD/etc.)
instead of stacking every section vertically in one tall window.

## The problem, measured

`PlayModeEditor::resized()` stacks, top to bottom, in one fixed-height
column: `PatchBrowserBar` (40px) -> the algorithm graph panel (graph view +
`OperatorEditorPanel`, 320px of which is the operator panel alone) ->
`ModSourceStrip` (140px) -> `FilterLfoPanel` (130px) -> `MacroStrip` (120px)
-> `FxChainStrip` (168px). That's ~960px of *fixed* allotments before the
algorithm graph itself, which explicitly "gets whatever's left rather than
a fixed share" (the code's own comment) -- on the current 980x1358 window,
that's another 300-400px handed to an 8-node circle whose actual job is
"select which node's controls show below." The window has already been
grown twice (GATE 5 -> GATE 6) to keep fitting more content in this same
one-screen-does-everything shape; growing it a third time isn't the fix,
restructuring is.

## Two separate work-streams

These are different-sized problems and don't need to ship together.

### 1. Algorithm graph -> a compact selectable row

Replace the circular 8-node diagram with a horizontal row of small,
clickable node chips -- same *function* (select a node, `OperatorEditorPanel`
below shows its controls, same click-to-select behavior UI GATE 3 already
built), radically less vertical space. Concretely: reuses the exact visual
language `OperatorEditorPanel`'s own engine-pill row already established
(a small labeled rectangle per option, filled/outlined by state) rather than
inventing a new visual idiom.

**What this trades away**: the current view's edge topology (which nodes
feed which, colored by `EdgeType`, per `docs/UI.md` "The algorithm graph
view") doesn't fit in a single compact row. Real options, needing an
explicit call rather than a silent default:

- **(a)** Drop edge visualization from PLAY mode entirely -- topology
  editing was already scoped as DESIGN mode's job, PLANNED
  (`docs/UI.md`); PLAY mode arguably never needed to *display* it either,
  just let you select a node.
- **(b)** Keep a minimal signal, e.g. a small connector tick between
  adjacent chips for a non-`Audio` edge type, or a tap/hover reveal --
  real design work, not just a resize.
- **(c)** Move the full circular view behind a "show graph" toggle/overlay,
  collapsed by default -- keeps the existing `AlgorithmGraphView` code
  entirely as-is, just changes when it's visible.

**Recommendation: (a) for v1.** It's the honest match for what the compact
row can actually show well, ships fastest, and doesn't block anything --
(b) or (c) can layer on later without touching the row itself.

### 2. Paged layout shell

The bigger structural change. A tab strip (Basic / OSC / Filter / Mod / FX,
see below) with one page visible at a time, replacing the vertical stack.
`PatchBrowserBar` (and the new compact graph row from work-stream 1) stay
persistent across every page -- they're global context, not
section-specific content.

**The good news: almost every page is an existing, already-built, already-
tested component, just re-homed into a page instead of a fixed vertical
slot.**

| Page | Component | Status |
|---|---|---|
| OSC | `OperatorEditorPanel` | Exists as-is |
| Filter | `FilterLfoPanel` | Exists as-is (currently bundles Filter1 + LFO1; consider splitting or renaming if Mod page absorbs LFO -- see open questions) |
| Mod | `ModSourceStrip` | Exists as-is |
| FX | `FxChainStrip` | Exists as-is |
| Basic | New composition (see below) | Partly new |
| *(gap)* Envelope | **No UI exists today at all** -- `ampEnvelope` has no PLAY-mode panel despite being a real, automatable parameter. Worth adding as its own page or folded into OSC while this restructure is already touching page boundaries, rather than leaving it the one section with literally no way to reach it. |

This means the paged shell itself -- tab strip, page-switching, persistent
header -- is the actual new engineering surface. The content inside most
pages is a relayout, not a rebuild.

## "Basic" view: the patch's own 6 knobs

The harder open question. Wanting "the patch dictates which knobs show, and
their labels" describes something this project has **already built**: the 8
macros (`patch::Macro`: `id`/`name`/`description`/`value`, each routable to
multiple destinations via the mod matrix, patch-authored name shown by
`MacroStrip` today -- `docs/MODULATION.md` "Macros", `docs/PATCH_BROWSER.md`
-adjacent framing). The request specifies **6**, not 8, which needs one
explicit decision before building:

- **(a)** Basic view shows all 8 macros (matches the existing schema and
  Phase Plant's "8 routable macros" model this project already committed
  to, per `docs/COMPETITIVE_ANALYSIS.md`) -- "6" was closer to "several,"
  not a hard requirement. Zero new data model.
- **(b)** Basic view shows only the *first 6 non-default-named* macros (a
  patch that only names 6 of its 8 macros implicitly signals "these are
  the ones that matter"), with the full 8 still reachable on a dedicated
  Macros/Mod page. Small new logic, no schema change.
- **(c)** A real reduction to exactly 6 macros -- a schema/engine change
  (shrinking `Patch::macros[8]`), touching `PluginState.h`'s parameter
  count and every place `kMacroParameterIds`/`Names` is read. Real work
  for a step *backward* in automation surface; only worth it if there's a
  concrete reason 8 is actually wrong, not just "the ask said 6."

**Recommendation: (a).** It's zero new engineering, matches a model
already validated against real competitors, and "Basic" can still *look*
like a clean 6-8 knob row regardless of the exact count -- the visual
promise ("a few big labeled knobs, not the whole synth") doesn't actually
require the number to be exactly 6.

## Sequencing

1. **Paged shell first, using existing components verbatim** (OSC/Filter/
   Mod/FX pages) -- the highest-leverage, lowest-risk slice: mostly moving
   already-built, already-tested components into a tab container. Ship
   this before touching the graph view at all; it alone probably resolves
   most of "too tall."
2. **Basic page**, decision (a) above -- reuses `MacroStrip` essentially
   as-is as the page's content.
3. **Compact graph row**, decision (a) above (drop edge display for now) --
   independent of 1-2, can land before, after, or in parallel.
4. **Envelope page/panel** -- real gap, opportunistic to close while page
   boundaries are already being decided, but not blocking on the above.
5. Edge-topology-in-compact-row (options (b)/(c) from work-stream 1) --
   explicitly deferred, not silently dropped.

## Relationship to DESIGN mode (don't conflate these)

`docs/UI.md` already scopes a separate, larger **DESIGN mode** (Phase 17,
PLANNED): graph *topology* editing (moving nodes, adding/removing edges),
a full mod-matrix editor, per-algorithm FX detail -- real patch-*design*
work, not performance. This plan is **not** that. It's PLAY mode
reorganizing its own existing, already-implemented controls into pages for
screen-space reasons; nothing here adds graph editing or new mod-matrix
capability. Building this paged shell first is likely to make DESIGN mode
easier later (proves out tab/page navigation once, DESIGN mode adds its own
pages to the same shell rather than inventing navigation twice) -- worth
noting, not a reason to scope-creep this plan into DESIGN mode's job now.
