#pragma once

#include <cmath>
#include <cstddef>

#include "pw8/dsp/Math.hpp"

// Envelope follower for AU sidechain input (EXT oscillator theory — follower MVP).
namespace pw8::dsp
{
    class SidechainFollower
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            reset();
        }

        void reset() noexcept
        {
            envelope_ = 0.0f;
            peakHold_ = 0.0f;
            peakDecay_ = 0.0f;
            active_ = false;
        }

        void processBlock(const float* left, const float* right, std::size_t numFrames) noexcept
        {
            if (left == nullptr || numFrames == 0)
            {
                tickRelease(numFrames);
                return;
            }

            const float attackCoeff = coeffForMs(12.0f);
            const float releaseCoeff = coeffForMs(280.0f);
            const float peakDecayCoeff = coeffForMs(450.0f);
            float blockPeak = 0.0f;

            for (std::size_t i = 0; i < numFrames; ++i)
            {
                const float r = right != nullptr ? right[i] : left[i];
                // RMS-ish mono sum — smoother than half-wave rectified peak.
                const float monoRaw = std::sqrt((left[i] * left[i] + r * r) * 0.5f);
                const float mono = flushIfNotFinite(monoRaw);
                blockPeak = std::max(blockPeak, mono);
                const float coeff = mono > envelope_ ? attackCoeff : releaseCoeff;
                envelope_ = mono + coeff * (envelope_ - mono);
                envelope_ = flushIfNotFinite(envelope_);
            }

            if (blockPeak >= peakHold_)
            {
                peakHold_ = blockPeak;
                peakDecay_ = blockPeak;
            }
            else
            {
                peakDecay_ = blockPeak + peakDecayCoeff * (peakDecay_ - blockPeak);
                peakHold_ = peakDecay_;
            }

            active_ = envelope_ > 1.0e-4f || peakHold_ > 1.0e-4f;
        }

        [[nodiscard]] float envelope() const noexcept { return envelope_; }

        [[nodiscard]] bool isActive() const noexcept { return active_; }

    private:
        void tickRelease(std::size_t numFrames) noexcept
        {
            const float releaseCoeff = coeffForMs(280.0f);
            const float peakDecayCoeff = coeffForMs(450.0f);
            for (std::size_t i = 0; i < numFrames; ++i)
            {
                envelope_ = releaseCoeff * envelope_;
                peakDecay_ = peakDecayCoeff * peakDecay_;
            }
            peakHold_ = peakDecay_;
            active_ = envelope_ > 1.0e-4f;
        }

        [[nodiscard]] float coeffForMs(float ms) const noexcept
        {
            const float tau = ms * 0.001f * static_cast<float>(sampleRate_);
            return tau > 1.0f ? std::exp(-1.0f / tau) : 0.0f;
        }

        double sampleRate_ = 48000.0;
        float envelope_ = 0.0f;
        float peakHold_ = 0.0f;
        float peakDecay_ = 0.0f;
        bool active_ = false;
    };

} // namespace pw8::dsp
