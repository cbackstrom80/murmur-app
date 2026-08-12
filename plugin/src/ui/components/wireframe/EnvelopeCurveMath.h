#pragma once

#include <cmath>

// Curve shaping for envelope wireframe previews — mirrors envelope::DahdsrEnvelope.
namespace pw8::plugin::ui::wireframe
{
    inline float envelopeShapeUp(float t, float curveShape) noexcept
    {
        if (curveShape <= 0.0f)
            return t;
        return 1.0f - std::pow(1.0f - t, 1.0f + curveShape);
    }

    inline float envelopeShapeDown(float t, float curveShape) noexcept
    {
        if (curveShape <= 0.0f)
            return t;
        return 1.0f - std::pow(1.0f - t, 1.0f / (1.0f + curveShape));
    }

    struct EnvelopePreviewParams
    {
        float delaySeconds = 0.0f;
        float attackSeconds = 0.005f;
        float holdSeconds = 0.0f;
        float decaySeconds = 0.2f;
        float sustainLevel = 0.7f;
        float releaseSeconds = 0.3f;
        float curveShape = 2.0f;
    };

} // namespace pw8::plugin::ui::wireframe
