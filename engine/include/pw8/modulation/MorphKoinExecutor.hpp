#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/patch/Patch.hpp"

// Frames-style morph between 2–4 keyframes (docs/MORPH_KOIN_SPEC.md Horizon 3).
namespace pw8::modulation
{
    namespace detail
    {
        [[nodiscard]] inline float morphCurve(float t, const std::string& curve) noexcept
        {
            t = dsp::clamp(t, 0.0f, 1.0f);
            if (curve == "step")
                return t >= 0.999f ? 1.0f : 0.0f;
            if (curve == "smooth")
            {
                // Raised cosine (smoothstep).
                return t * t * (3.0f - 2.0f * t);
            }
            return t; // linear
        }

        [[nodiscard]] inline std::size_t keyframeCount(const patch::MorphKoin& mk) noexcept
        {
            return mk.keyframes.size();
        }

        inline void ensureKeyframePositions(patch::MorphKoin& mk) noexcept
        {
            const std::size_t n = mk.keyframes.size();
            if (n < 2)
                return;

            bool anyUnset = false;
            for (const auto& kf : mk.keyframes)
            {
                if (kf.position < 0.0f || kf.position > 1.0f)
                    anyUnset = true;
            }
            if (!anyUnset && n >= 2)
                return;

            for (std::size_t i = 0; i < n; ++i)
            {
                if (n == 1)
                    mk.keyframes[i].position = 0.0f;
                else
                    mk.keyframes[i].position = static_cast<float>(i) / static_cast<float>(n - 1);
            }
        }

        [[nodiscard]] inline float lerp(float a, float b, float t) noexcept
        {
            return a + (b - a) * t;
        }

        inline void applyParamOverridePath(patch::Patch& patch, const std::string& path, float value) noexcept
        {
            if (path == "filterCutoffHz" || path == "layerA.filter1.cutoffHz")
            {
                patch.layerA.filter1.cutoffHz = value;
                return;
            }
            if (path == "layerA.filter1.resonance")
            {
                patch.layerA.filter1.resonance = value;
                return;
            }

            // masterEffects[N].field
            if (path.rfind("masterEffects[", 0) == 0)
            {
                const auto openBracket = path.find('[', 0);
                const auto closeBracket = path.find(']', openBracket);
                if (openBracket == std::string::npos || closeBracket == std::string::npos ||
                    closeBracket + 2 >= path.size())
                    return;
                int slot = 0;
                for (std::size_t i = openBracket + 1; i < closeBracket; ++i)
                {
                    const char c = path[i];
                    if (c < '0' || c > '9')
                        return;
                    slot = slot * 10 + (c - '0');
                }
                if (slot < 0 || slot >= static_cast<int>(effects::kNumMasterSlots))
                    return;
                const std::string field = path.substr(closeBracket + 2);
                auto& e = patch.masterEffects[static_cast<std::size_t>(slot)];

                if (field == "mix")
                    e.mix = value;
                else if (field == "qsr1Distance")
                    e.qsr1Distance = value;
                else if (field == "qsr2Distance")
                    e.qsr2Distance = value;
                else if (field == "qsr1Angle" || field == "qsr1AngleDeg")
                    e.qsr1AngleDeg = value;
                else if (field == "qsr2Angle" || field == "qsr2AngleDeg")
                    e.qsr2AngleDeg = value;
                else if (field == "qsr1Height")
                    e.qsr1Height = value;
                else if (field == "qsr2Height")
                    e.qsr2Height = value;
                else if (field == "qsr1RoomAmount")
                    e.qsr1RoomAmount = value;
                else if (field == "qsr2RoomAmount")
                    e.qsr2RoomAmount = value;
                else if (field == "cntrLevel")
                    e.cntrLevel = value;
                else if (field == "quasarDelayFeedback")
                    e.quasarDelayFeedback = value;
                else if (field == "quasarDelayTimeMs")
                    e.quasarDelayTimeMs = value;
                else if (field == "quasarDelayVolume")
                    e.quasarDelayVolume = value;
                else if (field == "qsr1RoomSize")
                    e.qsr1RoomSize = value;
                else if (field == "qsr2RoomSize")
                    e.qsr2RoomSize = value;
                else if (field == "qsr1RoomDamping")
                    e.qsr1RoomDamping = value;
                else if (field == "qsr2RoomDamping")
                    e.qsr2RoomDamping = value;
                else if (field == "qsr1Level")
                    e.qsr1Level = value;
                else if (field == "qsr2Level")
                    e.qsr2Level = value;
                return;
            }

            // Bare quasar param names — apply to first BinauralSpace master slot.
            for (auto& e : patch.masterEffects)
            {
                if (e.type != effects::EffectType::BinauralSpace)
                    continue;
                if (path == "qsr1Distance")
                    e.qsr1Distance = value;
                else if (path == "qsr2Distance")
                    e.qsr2Distance = value;
                else if (path == "qsr1RoomAmount")
                    e.qsr1RoomAmount = value;
                else if (path == "qsr2RoomAmount")
                    e.qsr2RoomAmount = value;
                else if (path == "cntrLevel")
                    e.cntrLevel = value;
                else if (path == "quasarDelayFeedback")
                    e.quasarDelayFeedback = value;
                else if (path == "quasarDelayTimeMs")
                    e.quasarDelayTimeMs = value;
                else if (path == "mix")
                    e.mix = value;
                break;
            }
        }
    } // namespace detail

    /// Interpolate morph keyframes at `position` and write macro baselines + param overrides into `patch`.
    inline void applyMorphKoin(patch::Patch& patch, float position) noexcept
    {
        auto& mk = patch.morphKoin;
        if (mk.keyframes.size() < 2)
            return;

        detail::ensureKeyframePositions(mk);

        auto sorted = mk.keyframes;
        std::sort(sorted.begin(), sorted.end(),
                  [](const patch::MorphKoinKeyframe& a, const patch::MorphKoinKeyframe& b) {
                      return a.position < b.position;
                  });

        float pos = dsp::clamp(position, 0.0f, 1.0f);
        if (mk.wrap && pos > sorted.back().position)
            pos = std::fmod(pos, 1.0f);

        std::size_t seg = 0;
        while (seg + 1 < sorted.size() && sorted[seg + 1].position < pos)
            ++seg;
        if (seg + 1 >= sorted.size())
            seg = sorted.size() - 2;

        const auto& kfA = sorted[seg];
        const auto& kfB = sorted[seg + 1];
        const float span = kfB.position - kfA.position;
        const float localT = span > 1.0e-6f ? (pos - kfA.position) / span : 0.0f;
        const float t = detail::morphCurve(localT, mk.curve);

        if (kfA.hasMacroValues || kfB.hasMacroValues)
        {
            for (std::size_t i = 0; i < patch.macros.size(); ++i)
            {
                const float a = kfA.hasMacroValues ? kfA.macroValues[i] : patch.macros[i].value;
                const float b = kfB.hasMacroValues ? kfB.macroValues[i] : a;
                patch.macros[i].value = detail::lerp(a, b, t);
            }
        }

        // Merge param override keys from both keyframes.
        for (const auto& [path, valA] : kfA.paramOverrides)
        {
            float valB = valA;
            const auto itB = kfB.paramOverrides.find(path);
            if (itB != kfB.paramOverrides.end())
                valB = itB->second;
            detail::applyParamOverridePath(patch, path, detail::lerp(valA, valB, t));
        }
        for (const auto& [path, valB] : kfB.paramOverrides)
        {
            if (kfA.paramOverrides.find(path) != kfA.paramOverrides.end())
                continue;
            detail::applyParamOverridePath(patch, path, valB);
        }

        mk.position = pos;
    }

} // namespace pw8::modulation
