#pragma once

#include <atomic>

#include <juce_core/juce_core.h>

#include "dsp/AudioTapBuffer.h"

namespace pw8::plugin
{
    /// Lock-free mono mix tap for UI analysis. Audio thread pushes; message thread reads.
    class ScopeAudioTap
    {
    public:
        static constexpr int kFifoCapacity = 32768;

        void reset() noexcept { tap_.reset(); }

        void pushStereoBlock(const float* left, const float* right, int numSamples) noexcept
        {
            if (left == nullptr || right == nullptr || numSamples <= 0)
                return;

            float blockLeftPeak = 0.0f;
            float blockRightPeak = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                blockLeftPeak = juce::jmax(blockLeftPeak, std::abs(left[i]));
                blockRightPeak = juce::jmax(blockRightPeak, std::abs(right[i]));
            }

            leftPeakLinear_.store(blockLeftPeak, std::memory_order_relaxed);
            rightPeakLinear_.store(blockRightPeak, std::memory_order_relaxed);
            tap_.pushMonoBlock(left, right, numSamples);
        }

        [[nodiscard]] int readMono(float* dest, int maxSamples) noexcept { return tap_.read(dest, maxSamples); }

        [[nodiscard]] float getLeftPeakLinear() const noexcept
        {
            return leftPeakLinear_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] float getRightPeakLinear() const noexcept
        {
            return rightPeakLinear_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] int getNumReady() const noexcept { return tap_.getNumReady(); }

    private:
        dsp::AudioTapBuffer<static_cast<std::size_t>(kFifoCapacity)> tap_{};
        std::atomic<float> leftPeakLinear_{0.0f};
        std::atomic<float> rightPeakLinear_{0.0f};
    };

} // namespace pw8::plugin
