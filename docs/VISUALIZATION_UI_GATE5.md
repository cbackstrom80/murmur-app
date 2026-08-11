# UI GATE 5: Visualization (wavetable stack shipped; spectrum/scope specced, not built)

Follows on from `docs/PLUGIN_ARCHITECTURE.md`'s "Visualization" section (the
three-surface plan: spectrum, oscilloscope, wavetable preview) and the "3D
visualization" ask that prompted this pass. Two different risk levels here, kept
deliberately separate:

## What this pass actually shipped

**`WavetableStackView`** (`plugin/src/ui/components/WavetableStackView.{h,cpp}`) --
a pseudo-3D "deck of cards" frame-stack view of the selected operator's loaded
wavetable. Real, compilable code (not build-verified in the environment this was
written in -- no cmake/JUCE toolchain available there; needs a real build before
merging, same caveat as every plugin-side change so far).

Deliberately **not** a real 3D engine: no `juce::OpenGLContext`, no shaders, no
camera/mesh. Every frame is a flat `juce::Path` with an `AffineTransform` shear,
decreasing alpha with depth -- reads as dimensional at this scale for a fraction of
the engineering cost and risk of an actual 3D renderer. This matches the "3D
visualization" request's most likely real payoff (a Serum-style wavetable stack,
not an orbit-able 3D scene) without inventing a new rendering subsystem the rest of
this skin doesn't have and doesn't need elsewhere.

**Needed one small, safe engine/processor addition** to make this possible at all:
`Engine::getWavetableTable(opIndex)` (new, `engine/include/pw8/render/Engine.hpp`)
and `PatchworkEightProcessor::getActiveWavetableTable(opIndex)` (new,
`plugin/src/processor/PatchworkEightProcessor.h`) -- read-only accessors exposing
data that already lives in memory once `loadPatch()` runs, no audio-thread
involvement. The processor-level accessor reads through the *same* atomic
`activeEngine_` pointer `processBlock()` reads; see its doc comment for why a
second, message-thread-only reader is safe against `publishEngine()`'s
double-buffer lifetime scheme specifically (both the swap and this read happen on
the message thread, so they're sequenced against each other regardless of what the
audio thread is doing).

## Integration (not wired into PlayModeEditor yet)

`PlayModeEditor`'s layout is already fully budgeted at 980x1178 (UI GATE 4). Rather
than force `WavetableStackView` into that sight-unseen, the honest options are:

1. **Swap it in for `OperatorEditorPanel`'s reserved area when the selected node's
   engine is `Wavetable`** -- same footprint, conditional content (pills+knobs for
   every other engine, the frame stack for this one). Lowest layout risk, but needs
   `OperatorEditorPanel` to own/host both and switch on `node.engine`.
2. **A new toggle/tab within the existing graph card** -- more UI surface, more
   design work, no layout budget change elsewhere.

Recommend (1). Not implemented here because it means restructuring
`OperatorEditorPanel`'s ownership of `WavetableStackView`, which is exactly the
kind of layout-affecting change that should go through a real build before
landing, not be guessed at blind a second time in the same pass.

## What's still just a plan: spectrum + oscilloscope

Both need a **realtime audio-thread tap** that doesn't exist anywhere in this
codebase yet -- this is the one place a mistake is actually dangerous (a glitch or
crash in someone's DAW session, not a misdrawn panel), so this stays a spec, not
code, until it can go through a real build/test cycle:

- **`AudioTapBuffer`** (new, `pw8_plugin`-only, not `pw8_core` -- doesn't touch the
  realtime-safety contract `docs/ARCHITECTURE.md` already commits `pw8_core` to): a
  fixed-capacity, lock-free single-producer/single-consumer ring buffer. Producer:
  `processBlock()`, writing the current block's post-fader output samples every
  call, no allocation, no locking -- matching the same realtime rules the engine
  itself already follows. Consumer: a UI-thread `juce::Timer` (~30Hz) that reads
  the latest window and repaints.
- **Spectrum analyzer**: the UI-thread timer runs `pw8::dsp::fft` (already
  implemented, `pw8/dsp/Fft.hpp`, built for wavetable mip generation, equally
  usable for analysis) over the tapped window and paints a magnitude spectrum.
- **Oscilloscope**: same tap, paints the raw time-domain window instead of an FFT.
- **`juce::OpenGLContext`** attached to the top-level editor accelerates repainting
  both at animation rate. Cross-vendor (OpenGL, not CUDA -- see
  `docs/GPU_ACCELERATION_RESEARCH.md`'s explicit distinction between DSP-compute
  GPU use, declined, and UI-rendering GPU use, this). Also benefits
  `WavetableStackView`'s repaints once it's wired in, and the algorithm graph's
  existing 24Hz pulse animation, though neither strictly needs it the way a live
  30Hz+ spectrum view does.

**Before any of this lands**: build it against a real JUCE checkout, verify the
ring buffer under an actual `processBlock()` load (ASan/UBSan the plugin build,
not just `pw8_core`'s existing sanitizer CI job, since this is new realtime code
those jobs don't cover today), and get it through at least one real DAW session
(ties into `docs/NEXT_STEPS.md`'s P0 "nothing has touched a real DAW yet"). This is
real engineering, not a follow-up polish pass -- sized similarly to Filter 1's
original implementation, not to a UI layout tweak.
