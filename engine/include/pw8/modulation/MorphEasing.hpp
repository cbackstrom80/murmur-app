#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "pw8/dsp/Math.hpp"

// Frames-inspired segment easing for morphKoin (docs/MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md).
// Curves match the Mutable Instruments Frames keyframer taxonomy; implementations are
// original closed-form math, not vendored LUT code from pichenettes/eurorack.
namespace pw8::modulation
{
    enum class MorphEasing : std::uint8_t
    {
        Linear = 0,
        Smooth,
        Step,
        InQuartic,
        OutQuartic,
        InOutSine,
        Bounce,
    };

    [[nodiscard]] inline float applyMorphEasing(float t, MorphEasing easing) noexcept
    {
        t = dsp::clamp(t, 0.0f, 1.0f);

        switch (easing)
        {
            case MorphEasing::Step:
                return t >= 0.5f ? 1.0f : 0.0f;
            case MorphEasing::Smooth:
                return t * t * (3.0f - 2.0f * t);
            case MorphEasing::InQuartic:
            {
                const float u = t * t;
                return u * u;
            }
            case MorphEasing::OutQuartic:
            {
                const float u = 1.0f - t;
                const float v = u * u;
                return 1.0f - v * v;
            }
            case MorphEasing::InOutSine:
                return 0.5f * (1.0f - std::cos(dsp::kPi * t));
            case MorphEasing::Bounce:
            {
                constexpr float n1 = 7.5625f;
                constexpr float d1 = 2.75f;
                if (t < 1.0f / d1)
                    return n1 * t * t;
                if (t < 2.0f / d1)
                {
                    const float u = t - 1.5f / d1;
                    return n1 * u * u + 0.75f;
                }
                if (t < 2.5f / d1)
                {
                    const float u = t - 2.25f / d1;
                    return n1 * u * u + 0.9375f;
                }
                const float u = t - 2.625f / d1;
                return n1 * u * u + 0.984375f;
            }
            case MorphEasing::Linear:
                break;
        }
        return t;
    }

    [[nodiscard]] inline MorphEasing parseMorphEasing(const std::string& curve) noexcept
    {
        if (curve == "smooth")
            return MorphEasing::Smooth;
        if (curve == "step")
            return MorphEasing::Step;
        if (curve == "inQuartic" || curve == "in_quartic" || curve == "accelerating")
            return MorphEasing::InQuartic;
        if (curve == "outQuartic" || curve == "out_quartic" || curve == "decelerating")
            return MorphEasing::OutQuartic;
        if (curve == "sine" || curve == "inOutSine" || curve == "in_out_sine")
            return MorphEasing::InOutSine;
        if (curve == "bounce")
            return MorphEasing::Bounce;
        return MorphEasing::Linear;
    }

    [[nodiscard]] inline float applyMorphEasing(float t, const std::string& curve) noexcept
    {
        return applyMorphEasing(t, parseMorphEasing(curve));
    }

    [[nodiscard]] inline const char* morphEasingLabel(MorphEasing easing) noexcept
    {
        switch (easing)
        {
            case MorphEasing::Linear: return "LIN";
            case MorphEasing::Smooth: return "SMO";
            case MorphEasing::Step: return "STP";
            case MorphEasing::InQuartic: return "IN4";
            case MorphEasing::OutQuartic: return "OUT4";
            case MorphEasing::InOutSine: return "SIN";
            case MorphEasing::Bounce: return "BNC";
        }
        return "LIN";
    }

    [[nodiscard]] inline MorphEasing cycleMorphEasing(MorphEasing easing) noexcept
    {
        return static_cast<MorphEasing>((static_cast<std::uint8_t>(easing) + 1) % 7);
    }

    [[nodiscard]] inline const char* morphEasingToString(MorphEasing easing) noexcept
    {
        switch (easing)
        {
            case MorphEasing::Smooth: return "smooth";
            case MorphEasing::Step: return "step";
            case MorphEasing::InQuartic: return "inQuartic";
            case MorphEasing::OutQuartic: return "outQuartic";
            case MorphEasing::InOutSine: return "sine";
            case MorphEasing::Bounce: return "bounce";
            case MorphEasing::Linear:
            default: return "linear";
        }
    }

} // namespace pw8::modulation
