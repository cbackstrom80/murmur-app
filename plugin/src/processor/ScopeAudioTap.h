#pragma once

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
            tap_.pushMonoBlock(left, right, numSamples);
        }

        [[nodiscard]] int readMono(float* dest, int maxSamples) noexcept { return tap_.read(dest, maxSamples); }

        [[nodiscard]] int getNumReady() const noexcept { return tap_.getNumReady(); }

    private:
        dsp::AudioTapBuffer<static_cast<std::size_t>(kFifoCapacity)> tap_{};
    };

} // namespace pw8::plugin
