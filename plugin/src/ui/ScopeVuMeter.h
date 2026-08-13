#pragma once

#include <cmath>
#include <utility>

#include <juce_audio_basics/juce_audio_basics.h>

namespace pw8::plugin::ui::scope
{
    inline constexpr float kVuMinDb = -60.0f;
    inline constexpr float kVuMaxDb = 0.0f;
    inline constexpr float kVuAttack = 0.72f;
    inline constexpr float kVuRelease = 0.085f;
    inline constexpr float kVuPeakHoldDecay = 0.982f;

    [[nodiscard]] inline float linearToVuNorm(float linear) noexcept
    {
        const float db = juce::Decibels::gainToDecibels(juce::jmax(1.0e-8f, linear), kVuMinDb);
        return juce::jlimit(0.0f, 1.0f, juce::jmap(db, kVuMinDb, kVuMaxDb, 0.0f, 1.0f));
    }

    [[nodiscard]] inline float asymmetricVuSmooth(float current, float target) noexcept
    {
        const float coeff = target > current ? kVuAttack : kVuRelease;
        return current + (target - current) * coeff;
    }

    [[nodiscard]] inline std::pair<float, float> measureMonoBlock(const float* samples, int count) noexcept
    {
        if (samples == nullptr || count <= 0)
            return {0.0f, 0.0f};

        float sumSq = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            const float s = samples[static_cast<std::size_t>(i)];
            sumSq += s * s;
            peak = juce::jmax(peak, std::abs(s));
        }

        const float rms = std::sqrt(sumSq / static_cast<float>(count));
        return {rms, peak};
    }

    struct VuBallistics
    {
        float rmsDisplay = 0.0f;
        float peakDisplay = 0.0f;
        float peakHold = 0.0f;

        void reset() noexcept
        {
            rmsDisplay = 0.0f;
            peakDisplay = 0.0f;
            peakHold = 0.0f;
        }

        void processFrame(float rmsLinear, float peakLinear) noexcept
        {
            const float rmsTarget = linearToVuNorm(rmsLinear);
            const float peakTarget = linearToVuNorm(peakLinear);

            rmsDisplay = asymmetricVuSmooth(rmsDisplay, rmsTarget);
            peakDisplay = asymmetricVuSmooth(peakDisplay, peakTarget);

            if (peakDisplay >= peakHold)
                peakHold = peakDisplay;
            else
                peakHold = juce::jmax(peakDisplay, peakHold * kVuPeakHoldDecay);
        }

        [[nodiscard]] float rmsNorm() const noexcept { return rmsDisplay; }
        [[nodiscard]] float peakNorm() const noexcept { return peakDisplay; }
        [[nodiscard]] float peakHoldNorm() const noexcept { return peakHold; }
    };

} // namespace pw8::plugin::ui::scope
