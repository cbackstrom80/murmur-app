#pragma once

#include <algorithm>
#include <cmath>

#include "pw8/dsp/Math.hpp"

namespace pw8::effects
{
    struct QuasarSpatialTargets
    {
        float qsr1AngleDeg = 30.0f;
        float qsr2AngleDeg = 330.0f;
        float qsr1Distance = 0.35f;
        float qsr2Distance = 0.40f;
        float qsr1Height = 0.0f;
        float qsr2Height = 0.0f;
    };

    struct QuasarMacroKoinValues
    {
        /// 0.5 = neutral. 0..1 rotates both feeds ±180° around the listener.
        float orbit = 0.5f;
        /// 0.5 = neutral. 0 = narrow, 1 = wide stereo image (angle + depth).
        float spread = 0.5f;
    };

    [[nodiscard]] inline float wrapDegrees360(float degrees) noexcept
    {
        float wrapped = std::fmod(degrees, 360.0f);
        if (wrapped < 0.0f)
            wrapped += 360.0f;
        return wrapped;
    }

    [[nodiscard]] inline QuasarSpatialTargets applyQuasarMacroKoins(const QuasarSpatialTargets& base,
                                                                  const QuasarMacroKoinValues& macros) noexcept
    {
        QuasarSpatialTargets out = base;

        const float orbitOffset = (dsp::clamp(macros.orbit, 0.0f, 1.0f) - 0.5f) * 360.0f;
        out.qsr1AngleDeg = wrapDegrees360(out.qsr1AngleDeg + orbitOffset);
        out.qsr2AngleDeg = wrapDegrees360(out.qsr2AngleDeg + orbitOffset);

        const float spreadNorm = dsp::clamp(macros.spread, 0.0f, 1.0f) - 0.5f;
        const float depthSpread = spreadNorm * 0.35f;
        const float heightSpread = spreadNorm * 0.25f;

        if (std::abs(spreadNorm) > 1.0e-4f)
        {
            const float midAngle = wrapDegrees360((out.qsr1AngleDeg + out.qsr2AngleDeg) * 0.5f);
            auto shortestSep = [](float a, float b) {
                const float delta = std::abs(wrapDegrees360(a - b));
                return std::min(delta, 360.0f - delta);
            };
            float separation = shortestSep(out.qsr1AngleDeg, out.qsr2AngleDeg);
            if (separation < 1.0f)
                separation = shortestSep(base.qsr1AngleDeg, base.qsr2AngleDeg);
            if (separation < 1.0f)
                separation = 60.0f;

            const float spreadScale = 1.0f + spreadNorm * 2.0f;
            separation = dsp::clamp(separation * spreadScale, 5.0f, 180.0f);
            out.qsr1AngleDeg = wrapDegrees360(midAngle - separation * 0.5f);
            out.qsr2AngleDeg = wrapDegrees360(midAngle + separation * 0.5f);
        }
        out.qsr1Distance = dsp::clamp(out.qsr1Distance + depthSpread, 0.0f, 1.0f);
        out.qsr2Distance = dsp::clamp(out.qsr2Distance + depthSpread, 0.0f, 1.0f);
        out.qsr1Height = dsp::clamp(out.qsr1Height + heightSpread, -1.0f, 1.0f);
        out.qsr2Height = dsp::clamp(out.qsr2Height - heightSpread, -1.0f, 1.0f);

        return out;
    }

} // namespace pw8::effects
