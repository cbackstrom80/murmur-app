#pragma once

#include <cstdint>

#include "pw8/core/Types.hpp"

// Mod matrix data model (docs/MODULATION.md "Mod Matrix"). Pure data -- no logic
// here, so this header is safe to include from pw8/patch/Patch.hpp without dragging
// anything heavier along. Execution lives in ModMatrixExecutor.hpp, consumed by
// pw8::voice::Voice.

namespace pw8::modulation
{
    enum class ModSource : std::uint8_t
    {
        None = 0,
        Lfo1,
        AmpEnvelope,
        Velocity,
        ChannelPressure,
        PolyAftertouch,
        MpeSlide,
        Macro1,
        Macro2,
        Macro3,
        Macro4,
        Macro5,
        Macro6,
        Macro7,
        Macro8,
    };

    enum class ModDestination : std::uint8_t
    {
        None = 0,
        FilterCutoff,   ///< exponential (semitone-style) offset, see ModMatrixExecutor.
        FilterResonance,
        OperatorLevel,  ///< requires `targetIndex` in [0, kNodesPerLayer).
        Pan,
    };

    /// Scope is recorded for forward compatibility with docs/MODULATION.md's
    /// VOICE/LAYER/GLOBAL model; only VOICE-scoped routes are executed in this pass
    /// (every route below runs per-voice, per-sample) -- see ModMatrixExecutor.
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
