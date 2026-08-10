#pragma once

#include "pw8/midi/MidiEvent.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/render/RenderTypes.hpp"

// Native offline renderer -- pw8_render's core, and the entry point Python bindings
// call into. Deliberately does NOT go through any JUCE/host infrastructure: this is
// the "render WITHOUT plugin hosting or DawDreamer" feature called out as a core
// requirement in docs/RENDERER.md.

namespace pw8::render
{
    class Engine;

    /// Convenience entry point: builds a fresh Engine, loads `patchToRender`, and renders.
    [[nodiscard]] RenderResult render(const patch::Patch& patchToRender, const midi::MidiSequence& midi,
                                       const RenderOptions& options) noexcept;

    /// Renders using an already-prepared Engine (already had prepare()/loadPatch() called).
    /// This is what lets a caller (e.g. the Python bindings' Engine.render()) reuse a
    /// live engine instance's already-loaded patch rather than reloading it per render.
    /// `engine` must already be prepared at `options.sampleRate`.
    [[nodiscard]] RenderResult renderWithEngine(Engine& engine, const midi::MidiSequence& midi,
                                                 const RenderOptions& options) noexcept;

} // namespace pw8::render
