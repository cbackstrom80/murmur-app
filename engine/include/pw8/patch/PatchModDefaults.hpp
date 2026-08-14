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

    [[nodiscard]] inline bool layerHasActiveMacroRoute(const LayerPatch& layer) noexcept
    {
        for (const auto& route : layer.modRoutes)
        {
            if (!route.isActive())
                continue;
            if (route.source >= modulation::ModSource::Macro1 && route.source <= modulation::ModSource::Macro8)
                return true;
        }
        return false;
    }

    /// Ensures at least one audible feature-macro KOIN route (Macro1 → filter) when a patch has
    /// no macro mod routes — safe on every load so Init and hand-authored patches still expose
    /// a working performance macro in Basic/Compact PLAY.
    inline void ensureMinimumMacroKoinRoutes(LayerPatch& layer) noexcept
    {
        if (layerHasActiveMacroRoute(layer))
            return;

        if (layer.modRoutes.size() + 2 > core::kMaxModRoutes)
            return;

        {
            modulation::ModRoute route{};
            route.source = modulation::ModSource::Macro1;
            route.destination = modulation::ModDestination::FilterCutoff;
            route.targetIndex = 0;
            route.amount = 18.0f;
            route.scope = modulation::ModScope::Voice;
            layer.modRoutes.push_back(route);
        }
        if (layer.modRoutes.size() < core::kMaxModRoutes)
        {
            modulation::ModRoute route{};
            route.source = modulation::ModSource::Macro1;
            route.destination = modulation::ModDestination::FilterResonance;
            route.targetIndex = 0;
            route.amount = 0.25f;
            route.scope = modulation::ModScope::Voice;
            layer.modRoutes.push_back(route);
        }
    }

} // namespace pw8::patch
