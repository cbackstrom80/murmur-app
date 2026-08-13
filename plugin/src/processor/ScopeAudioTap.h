#pragma once

#include <array>
#include <cmath>

#include <juce_audio_basics/juce_audio_basics.h>

namespace pw8::plugin
{
    /// Lock-free mono mix tap for UI spectrum analysis. Audio thread pushes; message thread reads.
    class ScopeAudioTap
    {
    public:
        static constexpr int kFifoCapacity = 32768;

        void reset() noexcept
        {
            fifo_.reset();
            buffer_.fill(0.0f);
        }

        void pushStereoBlock(const float* left, const float* right, int numSamples) noexcept
        {
            if (left == nullptr || right == nullptr || numSamples <= 0)
                return;

            int start1 = 0;
            int size1 = 0;
            int start2 = 0;
            int size2 = 0;
            fifo_.prepareToWrite(numSamples, start1, size1, start2, size2);

            const auto writeChunk = [&](int fifoStart, int count, int sampleOffset) {
                for (int i = 0; i < count; ++i)
                {
                    const auto idx = static_cast<std::size_t>(fifoStart + i);
                    buffer_[idx] = 0.5f * (left[sampleOffset + i] + right[sampleOffset + i]);
                }
            };

            writeChunk(start1, size1, 0);
            writeChunk(start2, size2, size1);
            fifo_.finishedWrite(size1 + size2);
        }

        [[nodiscard]] int readMono(float* dest, int maxSamples) noexcept
        {
            if (dest == nullptr || maxSamples <= 0)
                return 0;

            const int toRead = juce::jmin(maxSamples, fifo_.getNumReady());
            if (toRead <= 0)
                return 0;

            int start1 = 0;
            int size1 = 0;
            int start2 = 0;
            int size2 = 0;
            fifo_.prepareToRead(toRead, start1, size1, start2, size2);

            const auto readChunk = [&](int fifoStart, int count, int destOffset) {
                for (int i = 0; i < count; ++i)
                    dest[destOffset + i] = buffer_[static_cast<std::size_t>(fifoStart + i)];
            };

            readChunk(start1, size1, 0);
            readChunk(start2, size2, size1);
            fifo_.finishedRead(size1 + size2);
            return size1 + size2;
        }

        [[nodiscard]] int getNumReady() const noexcept { return fifo_.getNumReady(); }

    private:
        juce::AbstractFifo fifo_{kFifoCapacity};
        std::array<float, static_cast<std::size_t>(kFifoCapacity)> buffer_{};
    };

} // namespace pw8::plugin
