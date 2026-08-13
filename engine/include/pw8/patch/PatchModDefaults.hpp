#pragma once

#include "pw8/core/Types.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "pw8/patch/Patch.hpp"

namespace pw8::patch
{
    /// Default mod-wheel depth (semitones at full wheel) when a patch has no explicit route.
    inline constexpr float kDefaultModWheelFilterCutoffAmount = 24.0f;
    inline constexpr float kDefaultExpressionFilterResAmount = 0.35f;

    /// Ensures Layer A has at least one Mod Wheel → Filter Cutoff route. No-op if a mod-wheel
    /// route already exists (any destination). Safe to call on every load — factory presets and
    /// hand-authored patches without CC1 routing still respond musically to the mod wheel.
    inline void ensureDefaultModWheelRoute(LayerPatch& layer) noexcept
    {
        for (const auto& route : layer.modRoutes)
        {
            if (route.source == modulation::ModSource::ModWheel && route.isActive())
                return;
        }

        if (layer.modRoutes.size() >= core::kMaxModRoutes)
            return;

        modulation::ModRoute route{};
        route.source = modulation::ModSource::ModWheel;
        route.destination = modulation::ModDestination::FilterCutoff;
        route.targetIndex = 0;
        route.amount = kDefaultModWheelFilterCutoffAmount;
        route.scope = modulation::ModScope::Voice;
        layer.modRoutes.push_back(route);
    }

    /// Ensures Layer A has at least one Expression (CC11) → Filter Resonance route when none exists.
    inline void ensureDefaultExpressionRoute(LayerPatch& layer) noexcept
    {
        for (const auto& route : layer.modRoutes)
        {
            if (route.source == modulation::ModSource::Expression && route.isActive())
                return;
        }

        if (layer.modRoutes.size() >= core::kMaxModRoutes)
            return;

        modulation::ModRoute route{};
        route.source = modulation::ModSource::Expression;
        route.destination = modulation::ModDestination::FilterResonance;
        route.targetIndex = 0;
        route.amount = kDefaultExpressionFilterResAmount;
        route.scope = modulation::ModScope::Voice;
        layer.modRoutes.push_back(route);
    }

} // namespace pw8::patch
