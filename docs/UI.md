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
- `components/MacroStrip.{h,cpp}` -- the 8 macros, Patchwork Eight's actual
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
- `PlayModeEditor.{h,cpp}` -- the real `juce::AudioProcessorEditor`,
  `PatchworkEightProcessor::createEditor()`'s return value, replacing
  `juce::GenericAudioProcessorEditor`. Fixed-size, single screen: patch name
  strip -> algorithm graph (gets whatever vertical space is left after the
  three fixed-height utility strips below it are satisfied) -> Filter 1 + LFO
  1 -> the 8 macros -> the FX chain strip.

## The algorithm graph view

PLAY mode's centerpiece, and the component that actually differentiates this
synth visually: a read-only rendering of Layer A's live 8-node algorithm graph
-- the same structure `AlgorithmGraphCompiler` compiles and `pw8-graph
inspect` already prints in text form.

Deliberately **not** a draggable modular patcher: the master spec is explicit
this skin must never look like visible patch-cable spaghetti. The fix is
architectural, not just cosmetic -- the 8 nodes sit at fixed positions on a
circle (topology is schema data edited in DESIGN mode/`pw8-graph`, not a
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

Editing the graph (moving nodes, adding/removing edges) is explicitly out of
scope for PLAY mode -- DESIGN mode's job, PLANNED.

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

## What's PLANNED

- DESIGN and LAB modes (graph editing, full modulation-bank editing,
  per-algorithm FX detail, real preset browsing).
- The other 9 named skins.
- GPU-accelerated visuals (`juce::OpenGLContext`, no CUDA/compute dependency)
  for the graph view's animated pulses and a future spectrum/scope --
  deliberately not reached for in this pass; v1 uses plain `juce::Graphics`
  painting (already hardware-backed on macOS via CoreGraphics), profiled and
  found comfortably smooth, so there was nothing yet to accelerate.
- Real prev/next/save preset browsing (see `PatchBrowserBar` above).
- Per-algorithm FX detail editing beyond `mix` (see `FxChainStrip` above).

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
