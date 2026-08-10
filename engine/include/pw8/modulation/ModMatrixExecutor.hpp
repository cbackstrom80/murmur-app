#pragma once

#include <array>

#include "pw8/core/Types.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"

// Mod matrix execution (docs/MODULATION.md). Runs once per voice per sample inside
// pw8::voice::Voice::renderSample() -- see ModMatrixExecutor::apply(). No
// allocation: routes live in a fixed-capacity container
// (`core::FixedVector<ModRoute, core::kMaxModRoutes>` on LayerPatch), and this
// function only ever writes into a stack-allocated ModOutputs.
//
// Destination semantics:
//   FilterCutoff    -- `amount` is interpreted as semitones of cutoff shift per unit
//                       of source value (source values are -1..1 for bipolar sources
//                       like Lfo1, 0..1 for unipolar ones like AmpEnvelope/Velocity/
//                       macros). Applied exponentially: cutoffHz *= 2^(sum/12).
//   FilterResonance -- additive offset to resonance (0..1), summed then clamped.
//   OperatorLevel   -- multiplicative: level *= (1 + sourceValue * amount), per
//                       route, targeting `targetIndex` (0..7). Multiple routes to
//                       the same operator compose multiplicatively.
//   Pan             -- additive offset to layer pan (-1..1), summed then clamped.
//
// Only ModScope::Voice routes are meaningful in this pass -- Layer/Global-scoped
// execution (sharing one computed value across all voices in a layer, or across the
// whole patch) is PLANNED; a Layer/Global-scoped route is still read and applied,
// just at Voice scope, so it's not silently dropped -- see docs/MODULATION.md.

namespace pw8::modulation
{
    struct ModSourceValues
    {
        float lfo1 = 0.0f;             ///< -1..1
        float ampEnvelope = 0.0f;      ///< 0..1
        float velocity = 0.0f;         ///< 0..1
        float channelPressure = 0.0f;  ///< 0..1
        float polyAftertouch = 0.0f;   ///< 0..1
        float mpeSlide = 0.0f;         ///< 0..1
        std::array<float, 8> macros{}; ///< 0..1 each
    };

    struct ModOutputs
    {
        float filterCutoffSemitones = 0.0f;
        float filterResonanceOffset = 0.0f;
        std::array<float, core::kNodesPerLayer> operatorLevelMultiplier{};
        float panOffset = 0.0f;

        ModOutputs() noexcept { operatorLevelMultiplier.fill(1.0f); }
    };

    class ModMatrixExecutor
    {
    public:
        template <typename RouteContainer>
        [[nodiscard]] static ModOutputs apply(const RouteContainer& routes, const ModSourceValues& sources) noexcept
        {
            ModOutputs out;
            for (const auto& route : routes)
            {
                if (!route.isActive())
                    continue;

                const float sourceValue = resolveSource(route.source, sources);
                switch (route.destination)
                {
                    case ModDestination::FilterCutoff:
                        out.filterCutoffSemitones += sourceValue * route.amount;
                        break;
                    case ModDestination::FilterResonance:
                        out.filterResonanceOffset += sourceValue * route.amount;
                        break;
                    case ModDestination::OperatorLevel:
                    {
                        const std::uint8_t idx =
                            route.targetIndex < core::kNodesPerLayer ? route.targetIndex : std::uint8_t{0};
                        out.operatorLevelMultiplier[idx] *= (1.0f + sourceValue * route.amount);
                        break;
                    }
                    case ModDestination::Pan:
                        out.panOffset += sourceValue * route.amount;
                        break;
                    case ModDestination::None:
                        break;
                }
            }
            return out;
        }

    private:
        [[nodiscard]] static float resolveSource(ModSource src, const ModSourceValues& s) noexcept
        {
            switch (src)
            {
                case ModSource::Lfo1: return s.lfo1;
                case ModSource::AmpEnvelope: return s.ampEnvelope;
                case ModSource::Velocity: return s.velocity;
                case ModSource::ChannelPressure: return s.channelPressure;
                case ModSource::PolyAftertouch: return s.polyAftertouch;
                case ModSource::MpeSlide: return s.mpeSlide;
                case ModSource::Macro1: return s.macros[0];
                case ModSource::Macro2: return s.macros[1];
                case ModSource::Macro3: return s.macros[2];
                case ModSource::Macro4: return s.macros[3];
                case ModSource::Macro5: return s.macros[4];
                case ModSource::Macro6: return s.macros[5];
                case ModSource::Macro7: return s.macros[6];
                case ModSource::Macro8: return s.macros[7];
                case ModSource::None: return 0.0f;
            }
            return 0.0f;
        }
    };

} // namespace pw8::modulation
