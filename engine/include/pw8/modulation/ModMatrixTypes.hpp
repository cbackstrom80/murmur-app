#pragma once

#include <cstdint>

#include "pw8/core/Types.hpp"

// Mod matrix data model (docs/MODULATION.md "Mod Matrix"). Pure data -- no logic
// here, so this header is safe to include from pw8/patch/Patch.hpp without dragging
// anything heavier along. Execution lives in ModMatrixExecutor.hpp, consumed by
// pw8::voice::Voice.

namespace pw8::modulation
{
    /// 8 LFOs, 8 envelopes (envelope 1 is conventionally the amp envelope -- see
    /// LayerPatch::envelopes[0] -- but all 8 are fully general-purpose mod sources),
    /// 4 performance sources, and 8 macros -- docs/MODULATION.md "8 envelopes / 8
    /// LFOs" target, reached in the GATE 5 pass (docs/ROADMAP.md).
    enum class ModSource : std::uint8_t
    {
        None = 0,
        Lfo1, Lfo2, Lfo3, Lfo4, Lfo5, Lfo6, Lfo7, Lfo8,
        Env1, Env2, Env3, Env4, Env5, Env6, Env7, Env8,
        Velocity,
        ChannelPressure,
        PolyAftertouch,
        MpeSlide,
        Macro1, Macro2, Macro3, Macro4, Macro5, Macro6, Macro7, Macro8,
    };

    enum class ModDestination : std::uint8_t
    {
        None = 0,
        FilterCutoff,   ///< global filter; exponential (semitone-style) offset, see ModMatrixExecutor.
        FilterResonance,
        OperatorFilterCutoff,    ///< per-engine filter; requires `targetIndex` in [0, kNodesPerLayer).
        OperatorFilterResonance,
        OperatorLevel,           ///< requires `targetIndex` in [0, kNodesPerLayer).
        Pan,
        OperatorWavetablePosition, ///< requires `targetIndex`; additive offset, 0..1 result.
    };

    /// VOICE-scoped routes read a per-voice, independently-phased source (each
    /// voice's own LFO/envelope). LAYER and GLOBAL-scoped routes are only
    /// distinguished from VOICE for LFO sources: they instead read ONE shared,
    /// layer-wide LFO tick computed once per sample and identical across every
    /// voice in the layer (see `render::Engine`'s layer-scope LFO bank) -- a
    /// classic "one slow wobble shared by the whole chord" use case that a
    /// per-voice-independent LFO can't produce. Envelope/performance/macro sources
    /// are read at VOICE scope regardless of a route's declared scope: envelope
    /// LAYER/GLOBAL scope is intentionally not implemented (there's no single
    /// coherent trigger point for a "layer-wide envelope" when 0-32 notes can be
    /// overlapping independently -- see docs/MODULATION.md "Scope" for the full
    /// rationale), and macros are already global by construction. LAYER and
    /// GLOBAL currently behave identically (only Layer A is voiced, Phase 8) --
    /// see ModMatrixExecutor.
    enum class ModScope : std::uint8_t
    {
        Voice = 0,
        Layer,
        Global,
    };

    struct ModRoute
    {
        ModSource source = ModSource::None;
        ModDestination destination = ModDestination::None;
        /// Meaning depends on `destination`: for OperatorLevel, the node index (0-7).
        std::uint8_t targetIndex = 0;
        float amount = 0.0f;
        ModScope scope = ModScope::Voice;

        [[nodiscard]] bool isActive() const noexcept
        {
            return source != ModSource::None && destination != ModDestination::None;
        }
    };

} // namespace pw8::modulation
