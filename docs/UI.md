# UI

**PLAY mode: IMPLEMENTED (OBSIDIAN skin).** DESIGN/LAB modes: PLANNED.

Per the master spec, UI work was deliberately deferred until the DSP graph was
stable ("do not spend substantial UI time until DSP graph is stable") -- GATE
0-5/10/11 landing first is what made this pass legitimate. Per explicit user
direction ("1 skin but it has to be amazing"), this pass built exactly one
skin, all the way to genuinely finished, rather than scaffolding several.

## OBSIDIAN

The one launch skin, chosen as the safest path to "genuinely premium" over the
master spec's other 9 named skins (WALNUT/IVORY LAB/STAGE RED/COPPER NOIR/
SILVER MACHINE/BLUEPRINT/MONOLITH/BROADCAST/FUTURE CLASSIC, all PLANNED) --
dark glass/hardware territory already proven by u-he/Arturia/Serum's own dark
mode. Every color used anywhere in `plugin/src/ui/` comes from
`plugin/src/ui/theme/ObsidianPalette.h` -- no component hand-rolls a
`juce::Colour` literal, so the whole skin (or a second skin later) can be
re-tuned by editing one file. `ObsidianLookAndFeel` (`juce::LookAndFeel_V4`
subclass) is the only place any control's chrome is actually painted --
components configure a `juce::Slider`/`juce::ToggleButton`'s behavior and let
the LookAndFeel render it, so the visual language stays consistent by
construction rather than by convention.

Visual language: near-black charcoal cards with a real soft drop shadow (not
just a border -- see "Visual overhaul" below) set against a subtly grained,
vignetted background; a deliberate **duotone**, not a single accent -- cool
cyan for structural/signal things (the algorithm graph, Filter, FX), warm
amber for the one surface a player's hands are actually on (the 8 macros).
Neither reads as "extra" because each owns a distinct, consistent role rather
than competing for the same meaning -- restraint is still the point, just
spent on two colors instead of one. Anything "live" glows in its lane's
color -- an active knob's value arc/glow, the algorithm graph's output node
and traveling signal pulses -- restraint here is what separates "premium" from
"neon soup"; flat, precise "engineered" controls (a knob is a flat cylinder
with a thin indicator + glow arc, not skeuomorphic chrome).

## Architecture (`plugin/src/ui/`)

- `theme/ObsidianPalette.h` -- every color token.
- `theme/ObsidianLookAndFeel.{h,cpp}` -- `drawRotarySlider`/`drawToggleButton`/
  `getLabelFont`. Defensively floors its knob-geometry math against
  under-sized layouts (see "A real bug" below).
- `components/GlowKnob.{h,cpp}` -- the one control primitive: a rotary knob +
  name label + value readout, directly attached to an APVTS parameter via
  `SliderAttachment`. Takes an optional `valueToText` formatter so enum-valued
  parameters (filter mode, LFO waveform, ...) read as "LOWPASS"/"SINE", not a
  bare ordinal.
- `components/SectionPanel.{h,cpp}` -- the titled, recessed-panel container
  every PLAY-mode section sits inside.
- `components/AlgorithmGraphView.{h,cpp}` -- the centerpiece; see below.
- `components/OperatorEditorPanel.{h,cpp}` -- the detail view for whichever
  node is currently selected in the graph (UI GATE 3): engine-select pills,
  Waveform/Level/Ratio knobs.
- `components/ModSourceChip.{h,cpp}` / `components/ModSourceStrip.{h,cpp}` --
  the drag-to-modulate source palette and connections list (UI GATE 3).
- `components/MacroStrip.{h,cpp}` -- the 8 macros, MURMUR's actual
  performance surface.
- `components/FilterLfoPanel.{h,cpp}` -- Filter 1 + LFO 1's main controls,
  compact. The full 8-LFO/8-envelope bank and Filter 2 are DESIGN-mode
  territory (PLANNED), deliberately not duplicated here.
- `components/FxChainStrip.{h,cpp}` -- all 7 FX slots at a glance: which
  algorithm each is running (read-only, live-updating text -- type selection
  is a DESIGN-mode editing concern) plus the one control every non-Bypass
  algorithm shares, `mix`. Per-algorithm detail editing (Reverb's 15 fields,
  Eq's 7, ...) stays PLANNED for PLAY mode; every field remains
  host-automatable regardless (docs/PLUGIN_ARCHITECTURE.md "Automation").
- `components/PatchBrowserBar.{h,cpp}` -- current patch name only. Real
  prev/next/save browsing needs a preset-scanning system that doesn't exist
  yet (`content/presets/*.pw8` has no runtime index) -- honestly scoped to
  "shows what's loaded," not a fake browser. PLANNED.
- `PlayModeEditor.{h,cpp}` -- PLAY mode shell: **Basic** (patch focus + macros),
  **Advanced** (8-engine grid + FX rack dashboard + labs), **Compact** (320px HUD).
  Advanced default: `EngineGridPanel` (4×2 cards with per-engine OSC strip,
  Filter 1, ADSR, level/mix) over `DashboardStrip` (7-slot master FX chain +
  global filter only — LFO 1·2 and vocoder moved to dedicated labs) and
  `VstBottomBar` (VOCODER / LFO 1·2 / MOD launchers). Full-screen overlays:
  `VocoderLabPanel`, `DualLfoLabPanel`, `ModRoutingOverlay`, `EngineDetailOverlay`.
  Keyboard: `V` vocoder lab, `L` dual-LFO lab, `M` mod matrix, `Esc` dismiss.

## The algorithm graph view

PLAY mode's centerpiece, and the component that actually differentiates this
synth visually: a read-only rendering of Layer A's live 8-node algorithm graph
-- the same structure `AlgorithmGraphCompiler` compiles and `murmur-graph
inspect` already prints in text form.

Deliberately **not** a draggable modular patcher: the master spec is explicit
this skin must never look like visible patch-cable spaghetti. The fix is
architectural, not just cosmetic -- the 8 nodes sit at fixed positions on a
circle (topology is schema data edited in DESIGN mode/`murmur-graph`, not a
PLAY-mode drag target), so what's on screen is always a clean, readable
diagram of *this patch's specific graph*. Edges render as curved (bowed away
from center, so overlapping edges between non-adjacent nodes stay visually
separable) lines colored by `EdgeType`; a small dot travels along each edge, a
*structural* signal-flow indicator (which edges exist and what type), not
literal audio-level metering -- exposing real per-edge signal energy would
need a dedicated lock-free reporting path from the audio thread that doesn't
exist yet, so this is an honest, explicitly-scoped v1. Self-feedback edges
(`EdgeType::Feedback`, `source == destination`) draw as a small loop beside
the node instead. A node whose engine type isn't implemented yet
(`algorithm::isEngineImplemented()` false -- Additive/PhaseShape/Granular/
NoiseChaos/Resonator) gets a dashed ring instead of a solid one: an honest
visual cue that it renders silence today, not hidden behind a control that
looks fully live.

Editing the graph itself (moving nodes, adding/removing edges) is explicitly
out of scope for PLAY mode -- DESIGN mode's job, PLANNED. *Selecting* a node
is in scope as of UI GATE 3 below: clicking a node opens its controls in
`OperatorEditorPanel` without changing the graph's shape.

**PLANNED redesign:** [UI_PAGED_LAYOUT.md](UI_PAGED_LAYOUT.md) -- the circular
view has grown too tall across GATE 5/6's window-size increases; plans
collapsing it to a compact selectable row, and restructuring PLAY mode's
whole vertical stack (graph/operator/mod/filter/macro/FX, all visible at
once today) into a tabbed Basic/OSC/Filter/Mod/FX paged layout instead.

## UI GATE 2: visual overhaul

Per explicit user direction ("make it way radder," after reviewing a
reference mockup with much denser information architecture than PLAY mode
currently has), a second pass pushed the *visual* language further without
adding new feature surface -- the reference showed real value in typography,
depth, and duotone color, all buildable today; it also showed things this
project doesn't have DSP for yet (a GPU/spatial engine, 6 of 8 operator
engine types that still render silence, a live spectrum analyzer with no
audio-thread tap built) or that would explicitly violate the master spec
("CUDA GPU" contradicts its own "never require CUDA" rule) -- building
gorgeous chrome around those would have been dishonest, so this pass stayed
disciplined to what's real:

- **Typography**: `theme/ObsidianFonts.h`, a shared font helper every label/
  value/title routes through -- a curated system-font fallback chain (Avenir
  Next, preferred) rather than JUCE's plain default sans. The single biggest
  visual lever per unit of effort; a licensed, embedded, truly cross-platform
  typeface is the honest PLANNED follow-up once Windows/Linux builds are a
  real target.
- **Duotone**: `GlowKnob` gained an optional `accentColour` (wired via
  JUCE's own `Slider::rotarySliderFillColourId` rather than a parallel
  mechanism); `MacroStrip` is the only caller that uses it, painting the warm
  half.
- **Card depth**: `SectionPanel` gained a real soft drop shadow (three
  decreasing-alpha passes, not one hard-edged offset rectangle) and a faint
  top highlight -- the "milled panel set into a chassis" read now depends on
  both, not just a border. Also gained a card-header treatment (a small
  accent-colored dot + a hairline divider under the title row) -- visually
  closer to the reference's tab-style section headers without adding actual
  tab navigation, since PLAY mode still has exactly one screen.
- **Background**: `PlayModeEditor` gained a sparse, deterministic dot-grain
  texture (a cheap positional hash, not per-frame randomness, so it never
  swims between repaints) and a soft radial vignette pulling the eye toward
  the algorithm graph.
- **Ambient life**: the algorithm graph's output node(s) now have a slow
  breathing glow (sine-modulated alpha/radius) so the graph reads as alive
  even on the edge-less default init patch, not only when a signal pulse
  happens to be traveling an edge -- decorative, like the edge pulse itself,
  not driven by real audio levels.
- **Header**: `PatchBrowserBar` grew a proper two-line wordmark block
  ("PATCHWORK EIGHT" + "8-ENGINE ALGORITHMIC SYNTHESIZER"), matching the
  reference's branding treatment.

`auval` and `pluginval --strictness-level 5` both re-confirmed clean after
this pass (paint-only changes, no parameter/state-shape changes -- see
"Verification" below).

## A real bug, caught and fixed during this pass

The initial PLAY-mode layout budget starved `FxChainStrip` down to ~10px of
usable height (a fixed-height allocation scheme where the algorithm graph,
Filter/LFO, and macros all took a fixed slice first left too little for the
FX strip). `ObsidianLookAndFeel::drawRotarySlider`'s knob-geometry math (track
radius -> body radius, each derived from the previous by subtracting a
thickness term) went negative under that little room, producing a negative-
width/height `Graphics::fillEllipse` call. Caught via `lldb`, not guesswork:
breaking on `juce_GraphicsContext.cpp`'s internal `coordsToRectangle` bounds
check (conditioned on `w < 0 || h < 0`, since breaking unconditionally just
fires on every one of the thousands of legitimate calls) surfaced the exact
call site and the exact starved dimensions (`width=125, height=10`). Fixed
two ways, not one: the actual layout was rebalanced (the three utility strips
get fixed, generous heights; the graph panel gets whatever's left, not a
fixed share, so it can never starve them), *and* `drawRotarySlider` now
floors its diameter/body-radius math defensively, so a future layout mistake
or an unexpected host resize can't reproduce the same failure mode.

## UI GATE 3: click a node, drag a source -- graph legibility + drag-to-modulate

Built off direct feedback on the HTML mockup UI GATE 2 was prototyped in --
"why can't I click on other oscillators" and "I dont understand the graph" --
plus one piece of real friction found using the plugin inside an actual DAW
(REAPER) rather than the Standalone app for the first time. Three additions,
all in one pass:

- **Node selection**: `AlgorithmGraphView` gained a `mouseDown` hit-test over
  its 8 circle positions and an `onNodeSelected` callback; clicking a node
  opens that operator's controls in the new `OperatorEditorPanel` below the
  circle -- engine-select pills (mirroring the dashed-ring "not yet
  implemented" honesty the circle itself already commits to, dim/no-hover/
  tooltip rather than a control that looks live but silently no-ops) plus
  Waveform/Level/Ratio knobs, rebuilt (`showNode()`) against the newly
  selected node's parameter IDs rather than keeping 8 parallel hidden panels.
  One panel, one node's worth of live `GlowKnob` attachments at a time --
  there's only ever one selection.
- **Graph legibility**: the circle gained a reserved strip underneath for a
  plain-language caption (translating the current edge list into a sentence)
  and a legend mapping each `EdgeType` color actually present in *this* patch
  to its name -- both read off the same polled patch state the circle itself
  already has, not a separate data path.
- **Drag-to-modulate**: `ModSourceStrip` offers 3 colored, draggable
  `ModSourceChip`s (LFO 1, the amp envelope, Velocity) against the two real
  PLAY-mode mod-matrix destinations, Filter Cutoff/Resonance
  (`GlowKnob::enableModulationTarget`, a `juce::DragAndDropTarget` opt-in --
  not every knob accepts modulation, so this stays explicit rather than
  implicit on every `GlowKnob`). Dropping a chip calls
  `MurmurProcessor::setOrReplaceModRouteLive()`; right-clicking a
  modulated knob calls `removeModRouteLive()`. Both are message-thread APIs
  that publish into the live `Engine` via a double-buffered atomic-pointer
  handoff (`publishModRoutesLive()`/`pendingModRoutes_`, the same "prepare
  off-thread, swap a pointer" pattern the engine hot-swap already uses,
  applied to this one smaller piece of state) rather than a full
  `loadPatch()` rebuild -- so a drag never re-triggers or silences a
  currently-sustaining voice. `Engine::setModRoutesLive()` writes straight
  into `patch_.layerA.modRoutes`, which `Voice::renderSample` already reads
  fresh every sample (the same "no frozen per-voice copy" pattern
  `layerLfoValues` established) rather than a value frozen at `noteOn()`, so
  a route change reaches an already-held note the very next sample --
  proved directly in `tests/unit/EngineLiveParamsTests.cpp` (a mid-hold
  Velocity -> Filter Cutoff route measurably darkens the same still-ringing
  voice, and removing it measurably re-opens it). `ModSourceStrip`'s
  connections list reads every real route in the patch honestly (via
  `modSourceLabel`/`modDestinationLabel`), including ones this UI has no drag
  gesture for yet (e.g. an `OperatorLevel` route from a hand-authored .pw8),
  not just the two destinations reachable by dragging.
  The mod-route list itself remains outside the 578-parameter host-automation
  surface -- unchanged from `docs/PLUGIN_ARCHITECTURE.md`'s "Automation"
  section: adding/removing/retargeting a route is a discrete structural edit
  (like the graph topology it modulates), not a continuous knob value, so
  this is a new live-editing path alongside automation, not an addition to it.
- **`PatchBrowserBar` gained a real "Load..." button**: the first time this
  plugin was actually used inside REAPER rather than the Standalone app,
  there was no way to get a saved `.pw8` into a running instance short of a
  debug env-var hack. Opens a native `juce::FileChooser`, reads the chosen
  file's raw bytes, and calls `setStateInformation()` directly -- the exact
  same path a host already uses to restore a saved session, so "load a patch
  by hand" and "reopen a saved project" are provably the same code, not a
  second parallel loader to keep in sync.

`auval` and `pluginval --strictness-level 5` both re-confirmed SUCCESS on
VST3 and AU after this pass (still 578 published parameters -- mod routes
stay outside the automation surface, as above). 1 new engine-level test
(`EngineLiveParamsTests.cpp`) -- 126 total, all passing.

## UI GATE 4: FxChainStrip knob-starvation fix

A code-review pass (not mockup-driven, unlike GATEs 2/3) caught a real instance
of the exact "starved knob" failure mode UI GATE 1 fixed once already:
`FxChainStrip` shared `MacroStrip`'s fixed 120px panel height, but unlike
`MacroStrip` each of its 7 slots also reserves 24px above the knob for a
type-name label. That extra tax left `GlowKnob` only ~40px to work with --
after its own name-label/textbox subtraction, the rotary control floored out
at `ObsidianLookAndFeel`'s 16px defensive minimum, on all 7 slots at once.
16px is enough to not crash (the whole point of that floor), not enough to
read or grab. Fixed the same way as UI GATE 1: grew the strip's height (120 ->
168) and the window with it (+48, 980x1130 -> 980x1178 in isolation -- see the
UI GATE 5 note below for the actual final number once both gates' independent
window-height deltas are combined).

## UI GATE 5: wavetable stack view + OperatorEditorPanel knob-starvation fix

Prompted by a "make the visualization stuff radder / do 3D" request. Split into
what's actually cheap vs. a real project -- full breakdown in
`docs/VISUALIZATION_UI_GATE5.md`. What landed in PLAY mode:

- **`WavetableStackView`**: a pseudo-3D "deck of cards" frame-stack preview of
  the selected node's loaded wavetable (plain `juce::Path` + `AffineTransform`
  shear, no OpenGL/shaders/real 3D engine -- reads as dimensional at this scale
  for a fraction of the cost). Needs no audio-thread tap; wavetable data is
  already in memory once `loadPatch()` runs.
- Wired into `OperatorEditorPanel`: when the selected node's engine is
  Wavetable, the Wave/Ratio knobs (meaningless for that engine) are replaced by
  the stack view plus a new WT POS knob -- `WavetablePos` was a real
  automatable parameter with no UI anywhere until now. Level stays for every
  engine. A `Timer` (not just `showNode()`) drives the switch, since the engine
  can change without a node reselection (a different pill on the same node, or
  a host loading a different patch while this node stays selected).
- **`WavetableStackView` also owns a "Load..." button** -- the only UI anywhere
  that can actually assign a wavetable to an operator; previously the sole way
  in was hand-editing a `.pw8`'s `wavetableId` field, so picking the Wavetable
  engine on any node without one already baked into the loaded patch was a
  guaranteed dead end. Opens a native file chooser filtered to `*.json` (an
  already-built `murmur-wavetable-builder` table, not a raw `.wav` -- see
  `docs/PATCH_FORMAT.md`'s "Wavetable Resource Resolution"), then calls the new
  `MurmurProcessor::setOperatorWavetableFile()`, which sets
  `wavetableId` and reloads the patch (the only way to get a newly-picked file
  into the live `Engine`, since wavetable loading only happens inside
  `Engine::loadPatch()`). Build-verified end-to-end against the real
  Standalone app and the repo's own `content/wavetables/basic_harmonic.json`.
- **A second instance of UI GATE 4's exact bug, caught while touching this
  file**: `OperatorEditorPanel`'s Wave/Level/Ratio knobs were ALSO flooring out
  at `ObsidianLookAndFeel`'s 16px defensive minimum (its 140px allotment left
  only ~44px of content height for a 3-knob row after the pill row and note
  strip). Fixed the same way: grew the allotment (140 -> 190) and the window
  with it (+50), with the graph card's own height held constant by that same
  +50 -- the algorithm graph doesn't shrink to make room for this fix.
- Spectrum analyzer and oscilloscope stay unbuilt, specced only -- both need a
  new realtime audio-thread ring-buffer tap that doesn't exist anywhere in this
  codebase yet, the one place a mistake here is actually dangerous (a glitch or
  crash in a real DAW session). See `docs/VISUALIZATION_UI_GATE5.md`.

A follow-up pass on the same GATE fixed several smaller, independently-found
issues:

- **Tooltips actually work now.** `ObsidianLookAndFeel` themed
  `TooltipWindow::backgroundColourId`/`textColourId` from the start, but no
  `juce::TooltipWindow` instance existed anywhere to use them, and no
  `getTooltip()` existed to feed one. `PlayModeEditor` now owns a
  `TooltipWindow`; `OperatorEditorPanel` implements `juce::TooltipClient` so a
  disabled engine pill explains why it's dim on hover, hit-testing the same
  `pillBounds()` `mouseDown()` already uses rather than restructuring 8
  hand-painted pills into real child components.
- **Macro names read from the patch, not a hardcoded list.** `patch::Macro` has
  a real `name` field (a sound designer can call a macro "Growl" in the
  `.pw8`); `MacroStrip` previously always showed the generic "Macro 1"..."Macro
  8" regardless. Now polls and applies the patch-authored name once one's
  loaded, falling back to the generic name if it's empty.
- **`ModSourceStrip`'s instructional title now retires itself** once the player
  has successfully created at least one mod route -- "Mod Sources -- Drag Onto
  A Ringed Knob" switches to plain "Mod Sources" and stays there. The
  empty-state connections-list text still re-explains the gesture if every
  route later gets removed, so nothing's lost for a player who clears
  everything and comes back later.
- **`PatchBrowserBar`'s wordmark width** was a bare `280` independently
  duplicated in `paint()` and `resized()` -- harmless while they happened to
  agree, a real bug the first time only one got edited. Now a single named
  constant both read from.
- **Accessibility reviewed, not fixed** -- see the dedicated section above for
  what's free (real `juce::Component`-based controls) vs. what still needs
  real work (every hand-painted hit-target).

## UI GATE 6: WavetableStackView becomes a real 3D wireframe mesh

Prompted by a reference image (classic oscilloscope/wavetable-editor look:
perspective wireframe mesh, phosphor glow, real occlusion between rows) --
an explicit pivot away from an AI-image-generation approach tried first for
this same "make it cooler" ask (see below) and abandoned once it became
clear a diffusion model can't draw *this table's actual sample data*, only
a generic image that looks vaguely like a wireframe -- strictly less honest
than the view it would have sat behind.

What shipped instead is real, literal, procedural rendering, still just
`juce::Path` + hand-rolled projection math (no OpenGL, no AI, no network) --
the same constraint the original "deck of cards" ribbons committed to,
just a properly-projected mesh instead of a flat shear:

- **Visual density via honest interpolation**: draws more rows than the
  table's real frame count (as few as 1 in the factory library), each
  extra row's samples linearly interpolated between its two real
  neighbouring frames -- exactly what `WavetableOscillator` itself computes
  scanning `WavetablePos` between two frames, so the mesh changes exactly
  the way the audio does, not just decoratively.
- **Real occlusion**: drawn back-to-front; before a row's line is stroked,
  a silhouette (that row's own path, filled down to a baseline, in the
  panel's own background colour) erases whatever farther rows already
  drew in that footprint -- the classic painter's-algorithm hidden-line
  trick, validated against a Python prototype before porting to JUCE.
- **Cross-lines** tie each row to its farther neighbour at regular sample
  intervals -- what actually makes it read as one continuous mesh surface
  rather than a stack of independent parallel ribbons (the previous
  rendering's real shortfall against the reference).
- **Glow** without any image blur/convolution: each line strokes twice
  (wide+dim, then crisp+full-alpha), the same trick `ObsidianLookAndFeel`'s
  knob value-arc already uses.
- Colour stays OBSIDIAN's own `palette::kAccent` cyan, not the reference's
  literal green phosphor, so this reads as part of the existing skin.
- The row nearest the current `WavetablePos` gets the full-accent highlight,
  wherever it falls in the depth stack -- keeps the "always shows what's
  actually sounding" honesty property the original ribbons had.
- Panel/window grown again (opEditorArea 190 -> 320, window 980x1228 ->
  980x1358) -- a proper mesh needs real vertical room the previous
  allotment, sized for flat ribbons, didn't have. Same "grow the window,
  don't squeeze a control into illegibility" pattern every prior GATE used.

Also added, independent of the visual rework: **"<"/">" browse arrows**,
letting a player step through the other `*.json` tables sitting in the same
directory as the one currently assigned, without opening the file dialog
each time. Disabled until some `wavetableId` is already set (nothing to
browse siblings of otherwise). The "Load..." file-chooser button is
untouched -- both paths to assign a wavetable coexist.

**The abandoned AI-art detour**, for the record: tried generating background
art per table via a local FLUX model before landing on the mesh approach.
Two real, concrete things came out of trying it, beyond the aesthetic
conclusion above:
- A `diffusers`-based local FLUX server someone had built on this dev
  machine turned out not to fit in 24GB of unified memory unquantized --
  ballooned to 35GB resident and drove swap to nearly full before being
  killed. `mflux` (MLX-native, quantized) worked fine (8.58GB peak,
  reproducible via a real `--seed` flag) -- worth knowing for any future
  local-generation work on Apple Silicon: quantize, don't reach for the
  first Python package that runs the model.
- A prompt combining this project's own "dark"/"ambient" mood vocabulary
  (`docs/PATCH_FORMAT.md`) with FLUX.1-schnell produced images that were
  functionally pure black -- schnell's low/no-CFG 4-step generation didn't
  respond meaningfully to reworded exposure-floor instructions or even a
  different seed, a real limitation worth remembering if image generation
  comes up again for this project.

## Accessibility: partially free, partially not started

Every control built on a real `juce::Component` subclass gets JUCE's default
accessibility support for free -- `GlowKnob`'s `juce::Slider`, `PatchBrowserBar`'s
`juce::TextButton`, both already screen-reader/keyboard-navigable with no extra
work here. What's NOT accessible today: every hand-painted hit-target that isn't
a real child `Component` -- `OperatorEditorPanel`'s 8 engine pills,
`ModSourceChip`'s drag chips, `ModSourceStrip`'s per-route remove buttons,
`FxChainStrip`'s type labels. These exist only as `paint()` calls plus manual
`mouseDown()` hit-testing (the same pattern documented above for matching
`AlgorithmGraphView`'s style), which means zero keyboard focus and zero
screen-reader exposure for any of them. Fixing this for real means either turning
each into an actual child `Component` (bigger refactor, touches every hand-painted
control in the skin) or giving each parent a custom `juce::AccessibilityHandler`
exposing synthetic child elements (JUCE supports this, but it's real, unfamiliar-
in-this-codebase API surface that deserves a build to verify against real
assistive tech, not a guess). Left as PLANNED rather than attempted blind here --
UI GATE 5 did add real `getTooltip()` support (see above), which helps sighted
mouse users somewhat but doesn't move the needle on keyboard/screen-reader access
at all.

**Combined window size, both gates landed:** UI GATE 4's `fxArea` growth (+48)
and UI GATE 5's `opEditorArea` growth (+50) are independent deltas against
different strips -- they simply add. Final size once both are in `main`:
980x1228 (1130 + 48 + 50), not either gate's own in-isolation number above.

## What's PLANNED

- DESIGN and LAB modes (graph editing, full modulation-bank editing,
  per-algorithm FX detail, real preset browsing).
- The other 9 named skins.
- GPU-accelerated visuals (`juce::OpenGLContext`, no CUDA/compute dependency)
  for the graph view's animated pulses and a future spectrum/scope --
  deliberately not reached for in this pass; v1 uses plain `juce::Graphics`
  painting (already hardware-backed on macOS via CoreGraphics), profiled and
  found comfortably smooth, so there was nothing yet to accelerate.
- Real prev/next preset browsing (a content-scanning index over
  `content/presets/*.pw8`) -- `PatchBrowserBar` can now load any single file
  you pick (UI GATE 3), but still can't step through a library.
- Per-algorithm FX detail editing beyond `mix` (see `FxChainStrip` above).
- A full mod-matrix UI (all sources/destinations, multi-route-per-destination
  authoring) -- UI GATE 3's drag-to-modulate deliberately covers only 3
  sources x 2 destinations; everything else stays reachable only via a
  hand-authored `.pw8`.

## Considered and deliberately not adopted

[foleysfinest/PluginGuiMagic](https://github.com/ffAudio/PluginGuiMagic) --
a JUCE module for XML-driven, live-editable plugin GUIs with built-in
level-meter/analyser components. Not adopted for PLAY mode: the whole point of
this pass was a precisely bespoke, custom-painted visual language (exact glow/
arc knob rendering, the custom circular algorithm-graph layout), which a
generic-component system doesn't provide for free -- `AlgorithmGraphView` and
`GlowKnob` would still need to be fully custom `Component`s either way. Also
another third-party JUCE module to license-track alongside JUCE itself
(`THIRD_PARTY_LICENSES.md`). Worth a real look specifically for the PLANNED
GPU-accelerated spectrum/scope visualizers above, where its level-meter/
analyser components are a plausible accelerant -- not as a wholesale
replacement of what's already built and verified.
