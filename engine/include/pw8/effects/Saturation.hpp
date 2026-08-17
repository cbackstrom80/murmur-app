#pragma once

#include <cmath>

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// Drive-normalized waveshapers with dry/wet mix. Mode pills map to SaturationCharacter.
namespace pw8::effects
{
    [[nodiscard]] inline float saturateSample(float x, float driveLinear, int character) noexcept
    {
        const float drive = std::max(driveLinear, 1.0e-6f);
        switch (static_cast<SaturationCharacter>(dsp::clamp(character, 0, 4)))
        {
            case SaturationCharacter::Tape:
                return dsp::flushIfNotFinite(std::tanh(x * drive * 0.38f) * 0.92f + x * 0.04f);
            case SaturationCharacter::Diode:
                return dsp::flushIfNotFinite(dsp::clamp(x * (1.0f + drive * 0.25f), -1.0f, 1.0f));
            case SaturationCharacter::Fold:
                return dsp::flushIfNotFinite(std::sin(x * drive * 0.9f) * 0.75f);
            case SaturationCharacter::Crush:
            {
                const float steps = 2.0f + drive * 0.15f;
                return dsp::flushIfNotFinite(std::round(x * steps) / steps * 0.8f);
            }
            case SaturationCharacter::Tube:
            default:
                return dsp::softSaturate(x, drive);
        }
    }

    class SaturationProcessor
    {
    public:
        void prepare(double) noexcept {}
        void reset() noexcept {}

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float driveLinear = dsp::dbToGain(p.saturationDriveDb);
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            const int character = p.saturationCharacter;
            outL = dsp::lerp(inL, saturateSample(inL, driveLinear, character), mix);
            outR = dsp::lerp(inR, saturateSample(inR, driveLinear, character), mix);
        }
    };

} // namespace pw8::effects
