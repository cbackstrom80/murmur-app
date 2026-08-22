#pragma once

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

namespace pw8::effects
{
    /// Apply Plate/Hall/Room/Spring character bundles to a working copy of reverb params.
    inline EffectSlotParams applyReverbCharacter(const EffectSlotParams& p) noexcept
    {
        EffectSlotParams out = p;
        switch (static_cast<ReverbCharacter>(dsp::clamp(p.reverbCharacter, 0, 5)))
        {
            case ReverbCharacter::Plate:
                out.reverbSizeParam = dsp::clamp(out.reverbSizeParam * 0.72f, 0.2f, 3.0f);
                out.reverbDiffusion = dsp::clamp(out.reverbDiffusion + 0.18f, 0.0f, 1.0f);
                out.reverbDensity = dsp::clamp(out.reverbDensity + 0.08f, 0.0f, 1.0f);
                out.reverbModDepth = dsp::clamp(out.reverbModDepth * 0.65f, 0.0f, 1.0f);
                out.reverbEarlyLevel = dsp::clamp(out.reverbEarlyLevel * 0.55f, 0.0f, 1.0f);
                break;
            case ReverbCharacter::Hall:
                out.reverbSizeParam = dsp::clamp(out.reverbSizeParam * 1.45f, 0.2f, 3.0f);
                out.reverbDecaySeconds = dsp::clamp(out.reverbDecaySeconds * 1.25f, 0.05f, 20.0f);
                out.reverbDiffusion = dsp::clamp(out.reverbDiffusion + 0.12f, 0.0f, 1.0f);
                out.reverbHighRatio = dsp::clamp(out.reverbHighRatio * 0.85f, 0.2f, 1.0f);
                out.reverbEarlyLevel = dsp::clamp(out.reverbEarlyLevel + 0.08f, 0.0f, 1.0f);
                break;
            case ReverbCharacter::Room:
                out.reverbSizeParam = dsp::clamp(out.reverbSizeParam * 0.55f, 0.2f, 3.0f);
                out.reverbDecaySeconds = dsp::clamp(out.reverbDecaySeconds * 0.65f, 0.05f, 20.0f);
                out.reverbPreDelayMs = dsp::clamp(out.reverbPreDelayMs * 0.5f, 0.0f, 200.0f);
                out.reverbEarlyLevel = dsp::clamp(out.reverbEarlyLevel + 0.22f, 0.0f, 1.0f);
                out.reverbLateLevel = dsp::clamp(out.reverbLateLevel * 0.82f, 0.0f, 1.0f);
                break;
            case ReverbCharacter::Spring:
                out.reverbSizeParam = dsp::clamp(out.reverbSizeParam * 0.38f, 0.2f, 3.0f);
                out.reverbModDepth = dsp::clamp(out.reverbModDepth + 0.28f, 0.0f, 1.0f);
                out.reverbModRateHz = dsp::clamp(out.reverbModRateHz * 1.8f, 0.05f, 2.0f);
                out.reverbHighRatio = dsp::clamp(out.reverbHighRatio * 0.7f, 0.2f, 1.0f);
                out.reverbDiffusion = dsp::clamp(out.reverbDiffusion * 0.75f, 0.0f, 1.0f);
                break;
            case ReverbCharacter::Shimmer:
                // Real, relative remap, same style as the other 4: boost
                // whatever real shimmer amount the user already dialed in,
                // and lengthen/modulate the tail a bit so there's real time
                // for the ascending pitch-shifted wash to actually build up
                // and be heard, rather than decaying before it's audible.
                out.reverbShimmerAmount = dsp::clamp(out.reverbShimmerAmount + 0.4f, 0.0f, 1.0f);
                // 20.0f cap matches the other real characters' own real
                // convention (Hall/Room above), not the 30.0f UI range
                // FathomParamLayout.cpp separately allows for direct manual
                // control -- this remap only ever scales what's already
                // loaded, and fromJson's own real reverbDecaySeconds clamp
                // caps a *loaded* patch at 20.0f regardless.
                out.reverbDecaySeconds = dsp::clamp(out.reverbDecaySeconds * 1.6f, 0.05f, 20.0f);
                out.reverbModDepth = dsp::clamp(out.reverbModDepth * 1.3f, 0.0f, 1.0f);
                break;
            case ReverbCharacter::Default:
            default:
                break;
        }
        return out;
    }

} // namespace pw8::effects
