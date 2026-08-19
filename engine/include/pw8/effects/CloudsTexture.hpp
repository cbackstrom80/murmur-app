#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/effects/EffectTypes.hpp"

// Master-bus Clouds-style granular texture (Track G — MIT clean-room, docs §11).

namespace pw8::effects
{
    inline constexpr std::size_t kCloudsBufferSize = 192000; ///< ~4 s @ 48 kHz
    inline constexpr std::size_t kCloudsMaxGrains = 8;

    enum class CloudsMode : std::uint8_t
    {
        Granular = 0,
        LoopDelay,
        PitchShift,
    };

    struct CloudsGrain
    {
        float position = 0.0f;
        float increment = 1.0f;
        float envelope = 0.0f;
        float envInc = 0.0f;
        bool active = false;
    };

    class CloudsTexture
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            bufferL_.fill(0.0f);
            bufferR_.fill(0.0f);
            writePos_ = 0;
            for (auto& g : grains_)
                g.active = false;
            densityPhase_ = 0.0f;
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            if (mix <= 0.0f)
            {
                outL = inL;
                outR = inR;
                return;
            }

            bufferL_[writePos_] = inL;
            bufferR_[writePos_] = inR;
            writePos_ = (writePos_ + 1) % kCloudsBufferSize;

            const float density = dsp::clamp(p.cloudsDensity, 0.0f, 1.0f);
            const float grainMs = dsp::clamp(p.cloudsGrainSizeMs, 5.0f, 500.0f);
            const float pitch = dsp::clamp(p.cloudsPitch, 0.25f, 4.0f);
            const bool freeze = p.cloudsFreeze > 0.5f;

            densityPhase_ += density * 0.02f;
            if (densityPhase_ >= 1.0f)
            {
                densityPhase_ -= 1.0f;
                spawnGrain(grainMs, pitch, freeze);
            }

            float wetL = 0.0f;
            float wetR = 0.0f;
            for (auto& grain : grains_)
            {
                if (!grain.active)
                    continue;

                const std::size_t idx =
                    static_cast<std::size_t>(grain.position) % kCloudsBufferSize;
                wetL += bufferL_[idx] * grain.envelope;
                wetR += bufferR_[idx] * grain.envelope;

                grain.position += grain.increment;
                grain.envelope += grain.envInc;
                if (grain.envelope <= 0.0f)
                    grain.active = false;
            }

            outL = dsp::lerp(inL, wetL, mix);
            outR = dsp::lerp(inR, wetR, mix);
        }

    private:
        void spawnGrain(float grainMs, float pitch, bool freeze) noexcept
        {
            for (auto& grain : grains_)
            {
                if (grain.active)
                    continue;

                const float readOffset =
                    freeze ? static_cast<float>(writePos_)
                           : static_cast<float>(rng_.nextU64() % kCloudsBufferSize);
                grain.position = readOffset;
                grain.increment = pitch;
                const float grainSamples = grainMs * 0.001f * static_cast<float>(sampleRate_);
                grain.envelope = 1.0f;
                grain.envInc = -1.0f / dsp::clamp(grainSamples, 64.0f, static_cast<float>(kCloudsBufferSize));
                grain.active = true;
                return;
            }
        }

        double sampleRate_ = 48000.0;
        std::array<float, kCloudsBufferSize> bufferL_{};
        std::array<float, kCloudsBufferSize> bufferR_{};
        std::size_t writePos_ = 0;
        std::array<CloudsGrain, kCloudsMaxGrains> grains_{};
        float densityPhase_ = 0.0f;
        dsp::DeterministicRng rng_{0xC10D05ULL};
    };

} // namespace pw8::effects
