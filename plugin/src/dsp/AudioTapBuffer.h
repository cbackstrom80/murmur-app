#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace pw8::plugin::dsp
{
    /// Lock-free single-producer / single-consumer ring buffer for UI audio taps.
    /// Producer: audio thread (`processBlock`). Consumer: message thread (scope repaint).
    template <std::size_t Capacity>
    class AudioTapBuffer
    {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

    public:
        void reset() noexcept
        {
            writeIndex_.store(0, std::memory_order_relaxed);
            readIndex_.store(0, std::memory_order_relaxed);
        }

        void push(const float* samples, int numSamples) noexcept
        {
            if (samples == nullptr || numSamples <= 0)
                return;
            for (int i = 0; i < numSamples; ++i)
                pushOne(samples[i]);
        }

        void pushMonoBlock(const float* left, const float* right, int numSamples) noexcept
        {
            if (left == nullptr || right == nullptr || numSamples <= 0)
                return;
            for (int i = 0; i < numSamples; ++i)
                pushOne(0.5f * (left[i] + right[i]));
        }

        [[nodiscard]] int read(float* dest, int maxSamples) noexcept
        {
            if (dest == nullptr || maxSamples <= 0)
                return 0;

            int count = 0;
            while (count < maxSamples)
            {
                float sample = 0.0f;
                if (!readOne(sample))
                    break;
                dest[count++] = sample;
            }
            return count;
        }

        [[nodiscard]] int getNumReady() const noexcept
        {
            const auto w = writeIndex_.load(std::memory_order_acquire);
            const auto r = readIndex_.load(std::memory_order_relaxed);
            return static_cast<int>(w - r);
        }

    private:
        void pushOne(float sample) noexcept
        {
            const auto w = writeIndex_.load(std::memory_order_relaxed);
            buffer_[w & (Capacity - 1)] = sample;
            writeIndex_.store(w + 1, std::memory_order_release);
        }

        [[nodiscard]] bool readOne(float& out) noexcept
        {
            const auto r = readIndex_.load(std::memory_order_relaxed);
            const auto w = writeIndex_.load(std::memory_order_acquire);
            if (r == w)
                return false;

            out = buffer_[r & (Capacity - 1)];
            readIndex_.store(r + 1, std::memory_order_relaxed);
            return true;
        }

        std::array<float, Capacity> buffer_{};
        std::atomic<std::size_t> writeIndex_{0};
        std::atomic<std::size_t> readIndex_{0};
    };

} // namespace pw8::plugin::dsp
