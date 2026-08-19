#pragma once

#include <cmath>
#include <cstdint>

#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::modulation
{
    /// Per-route source shaping applied in ModMatrixExecutor before `amount` scaling.
    /// Magnitude is warped on [0, 1]; sign is preserved for bipolar sources (-1..1).
    [[nodiscard]] inline float shapeModSource(float sourceValue, ModCurve curve) noexcept
    {
        if (curve == ModCurve::Linear)
            return sourceValue;

        const float sign = sourceValue >= 0.0f ? 1.0f : -1.0f;
        const float magnitude = std::abs(sourceValue);

        switch (curve)
        {
            case ModCurve::Exponential:
                return sign * (magnitude * magnitude);
            case ModCurve::Logarithmic:
                return sign * std::sqrt(magnitude);
            case ModCurve::SCurve:
            {
                const float s = magnitude * magnitude * (3.0f - 2.0f * magnitude);
                return sign * s;
            }
            case ModCurve::Linear:
                break;
        }
        return sourceValue;
    }

    [[nodiscard]] inline const char* modCurveLabel(ModCurve curve) noexcept
    {
        switch (curve)
        {
            case ModCurve::Linear: return "LIN";
            case ModCurve::Exponential: return "EXP";
            case ModCurve::Logarithmic: return "LOG";
            case ModCurve::SCurve: return " S ";
        }
        return "LIN";
    }

    [[nodiscard]] inline ModCurve cycleModCurve(ModCurve curve) noexcept
    {
        return static_cast<ModCurve>((static_cast<std::uint8_t>(curve) + 1) % 4);
    }

} // namespace pw8::modulation
