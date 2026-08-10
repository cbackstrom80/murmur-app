#pragma once

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// The plainest slot in the bank: drive-normalized tanh soft clipping, dry/wet
// mixed. No memory, no history -- exists so a slot can add harmonic weight
// without reaching for a delay-family effect. See docs/FX_BANK.md.
namespace pw8::effects
{
    class SaturationProcessor
    {
    public:
        void prepare(double) noexcept {}
        void reset() noexcept {}

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float driveLinear = dsp::dbToGain(p.saturationDriveDb);
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = dsp::lerp(inL, dsp::softSaturate(inL, driveLinear), mix);
            outR = dsp::lerp(inR, dsp::softSaturate(inR, driveLinear), mix);
        }
    };

} // namespace pw8::effects
