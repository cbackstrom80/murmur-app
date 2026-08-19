#pragma once

#include <cmath>
#include <utility>

#include "pw8/dsp/Math.hpp"
#include "pw8/filter/CharacterFilter.hpp"

// Blades-inspired dual-filter routing morph (Track B-M2).
namespace pw8::filter
{
    /// F2 cutoff tracks F1 modulated cutoff + semitone offset when F1 is active; otherwise absolute F2 cutoff.
    [[nodiscard]] inline float computeFilter2CutoffHz(bool filter1Enabled, float filter1ModulatedCutoffHz,
                                                      const CharacterFilterParams& filter2,
                                                      float effectiveFrequencyHz) noexcept
    {
        if (filter1Enabled)
            return filter1ModulatedCutoffHz * std::pow(2.0f, filter2.cutoffOffsetSemitones / 12.0f);

        const float keyTrackFactor = dsp::filterKeyTrackFactor(effectiveFrequencyHz, filter2.keyTrack);
        return filter2.cutoffHz * keyTrackFactor;
    }

    /// Morph routing 0..1: serial F1→F2 → parallel → crossfade of both serial orders.
    template <typename RenderF1, typename RenderF2>
    [[nodiscard]] float applyDualFilterRouting(float input, float routing, bool filter1Enabled, bool filter2Enabled,
                                               RenderF1&& renderF1, RenderF2&& renderF2) noexcept
    {
        if (!filter1Enabled && !filter2Enabled)
            return input;
        if (filter1Enabled && !filter2Enabled)
            return renderF1(input);
        if (!filter1Enabled && filter2Enabled)
            return renderF2(input);

        const float r = dsp::clamp(routing, 0.0f, 1.0f);
        const float f1Out = renderF1(input);
        const float f2Out = renderF2(input);
        const float serialF1F2 = renderF2(f1Out);
        const float serialF2F1 = renderF1(f2Out);
        const float parallel = 0.5f * (f1Out + f2Out);
        const float crossfade = 0.5f * (serialF1F2 + serialF2F1);

        if (r <= 0.5f)
        {
            const float t = r * 2.0f;
            return serialF1F2 + (parallel - serialF1F2) * t;
        }

        const float t = (r - 0.5f) * 2.0f;
        return parallel + (crossfade - parallel) * t;
    }

} // namespace pw8::filter
