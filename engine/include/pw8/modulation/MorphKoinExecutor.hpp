#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "pw8/core/Types.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/modulation/MorphEasing.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"
#include "pw8/patch/Patch.hpp"

// Frames-style morph between keyframes (docs/MORPH_KOIN_SPEC.md, MI integration Track A).
namespace pw8::modulation
{
    namespace detail
    {
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
                mk.keyframes[i].position = n == 1 ? 0.0f : static_cast<float>(i) / static_cast<float>(n - 1);
        }

        [[nodiscard]] inline float lerp(float a, float b, float t) noexcept
        {
            return a + (b - a) * t;
        }

        [[nodiscard]] inline float lerpWithResponse(float a, float b, float t, const std::string& response) noexcept
        {
            if (response == "log" && a > 0.0f && b > 0.0f)
            {
                const float logA = std::log(a);
                const float logB = std::log(b);
                return std::exp(logA + (logB - logA) * t);
            }
            return lerp(a, b, t);
        }

        [[nodiscard]] inline float segmentEase(float localT, const patch::MorphParamOverride& a,
                                               const patch::MorphParamOverride& b,
                                               const std::string& globalCurve) noexcept
        {
            if (!a.easing.empty())
                return applyMorphEasing(localT, a.easing);
            if (!b.easing.empty())
                return applyMorphEasing(localT, b.easing);
            return applyMorphEasing(localT, globalCurve);
        }

        [[nodiscard]] inline float lerpOverride(const patch::MorphParamOverride& a,
                                                const patch::MorphParamOverride& b, float localT,
                                                const std::string& globalCurve) noexcept
        {
            const float t = segmentEase(localT, a, b, globalCurve);
            const std::string response = !a.response.empty() ? a.response : b.response;
            return lerpWithResponse(a.value, b.value, t, response);
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
            if (path == "layerA.filter2.cutoffHz")
            {
                patch.layerA.filter2.cutoffHz = value;
                return;
            }
            if (path == "layerA.filter2.drive")
            {
                patch.layerA.filter2.drive = dsp::clamp(value, 0.0f, 1.0f);
                return;
            }
            if (path.rfind("layerA.operators[", 0) == 0)
            {
                const auto openBracket = path.find('[', 0);
                const auto closeBracket = path.find(']', openBracket);
                if (closeBracket == std::string::npos || closeBracket + 2 >= path.size())
                    return;
                int op = 0;
                for (std::size_t i = openBracket + 1; i < closeBracket; ++i)
                {
                    const char c = path[i];
                    if (c < '0' || c > '9')
                        return;
                    op = op * 10 + (c - '0');
                }
                if (op < 0 || op >= static_cast<int>(core::kNodesPerLayer))
                    return;
                const std::string field = path.substr(closeBracket + 2);
                auto& operatorPatch = patch.layerA.operators[static_cast<std::size_t>(op)];
                if (field == "wavetableFramePosition")
                    operatorPatch.wavetableFramePosition = dsp::clamp(value, 0.0f, 1.0f);
                return;
            }

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
                return;
            }
        }
    } // namespace detail

    /// Map autoplay source name to normalized morph position 0..1.
    [[nodiscard]] inline float resolveAutoplayMorphPosition(const std::string& sourceName,
                                                            const ModSourceValues& sources) noexcept
    {
        if (sourceName.empty() || sourceName == "none")
            return -1.0f;

        if (sourceName == "modWheel")
            return dsp::clamp(sources.modWheel, 0.0f, 1.0f);
        if (sourceName == "expression")
            return dsp::clamp(sources.expression, 0.0f, 1.0f);
        if (sourceName == "sidechain")
            return dsp::clamp(sources.sidechain, 0.0f, 1.0f);

        if (sourceName.size() >= 4 && sourceName.rfind("lfo", 0) == 0)
        {
            int index = sourceName[3] - '1';
            if (index >= 0 && index < 8)
                return dsp::clamp((sources.layerLfos[static_cast<std::size_t>(index)] + 1.0f) * 0.5f, 0.0f, 1.0f);
        }
        return -1.0f;
    }

    /// Returns keyframe index crossed when moving from `prev` to `next`, or -1 if none.
    [[nodiscard]] inline int detectMorphKeyframeCrossing(const patch::MorphKoin& mk, float prev,
                                                         float next) noexcept
    {
        if (mk.keyframes.size() < 2)
            return -1;

        auto sorted = mk.keyframes;
        std::sort(sorted.begin(), sorted.end(),
                  [](const patch::MorphKoinKeyframe& a, const patch::MorphKoinKeyframe& b) {
                      return a.position < b.position;
                  });

        if (prev <= next)
        {
            for (std::size_t i = 0; i < sorted.size(); ++i)
            {
                const float p = sorted[i].position;
                if (prev < p && next >= p)
                    return static_cast<int>(i);
            }
        }
        else
        {
            for (std::size_t i = sorted.size(); i-- > 0;)
            {
                const float p = sorted[i].position;
                if (prev > p && next <= p)
                    return static_cast<int>(i);
            }
        }
        return -1;
    }

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
        const float t = applyMorphEasing(localT, mk.curve);

        if (kfA.hasMacroValues || kfB.hasMacroValues)
        {
            for (std::size_t i = 0; i < patch.macros.size(); ++i)
            {
                const float a = kfA.hasMacroValues ? kfA.macroValues[i] : patch.macros[i].value;
                const float b = kfB.hasMacroValues ? kfB.macroValues[i] : a;
                patch.macros[i].value = detail::lerp(a, b, t);
            }
        }

        for (const auto& [path, ovA] : kfA.paramOverrides)
        {
            patch::MorphParamOverride ovB = ovA;
            if (const auto itB = kfB.paramOverrides.find(path); itB != kfB.paramOverrides.end())
                ovB = itB->second;
            detail::applyParamOverridePath(patch, path,
                                           detail::lerpOverride(ovA, ovB, localT, mk.curve));
        }
        for (const auto& [path, ovB] : kfB.paramOverrides)
        {
            if (kfA.paramOverrides.find(path) != kfA.paramOverrides.end())
                continue;
            detail::applyParamOverridePath(patch, path, ovB.value);
        }

        mk.position = pos;
    }

} // namespace pw8::modulation
