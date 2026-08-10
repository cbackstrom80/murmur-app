#pragma once

#include <cstddef>

// Small, framework-independent audio buffer views.
// pw8_core never depends on juce::AudioBuffer.

namespace pw8::core
{
    /// Non-owning view over a single channel of contiguous float samples.
    class MonoBlockView
    {
    public:
        constexpr MonoBlockView() noexcept = default;
        constexpr MonoBlockView(float* data, std::size_t numFrames) noexcept
            : data_(data), numFrames_(numFrames) {}

        [[nodiscard]] constexpr float* data() const noexcept { return data_; }
        [[nodiscard]] constexpr std::size_t numFrames() const noexcept { return numFrames_; }

        [[nodiscard]] constexpr float& operator[](std::size_t i) const noexcept { return data_[i]; }

        constexpr void clear() const noexcept
        {
            for (std::size_t i = 0; i < numFrames_; ++i)
                data_[i] = 0.0f;
        }

    private:
        float* data_ = nullptr;
        std::size_t numFrames_ = 0;
    };

    /// Non-owning planar stereo view (separate left/right pointers, not interleaved).
    /// This is the primary type passed into and out of the realtime render path.
    class StereoBlockView
    {
    public:
        constexpr StereoBlockView() noexcept = default;
        constexpr StereoBlockView(float* left, float* right, std::size_t numFrames) noexcept
            : left_(left), right_(right), numFrames_(numFrames) {}

        [[nodiscard]] constexpr MonoBlockView left() const noexcept { return {left_, numFrames_}; }
        [[nodiscard]] constexpr MonoBlockView right() const noexcept { return {right_, numFrames_}; }
        [[nodiscard]] constexpr std::size_t numFrames() const noexcept { return numFrames_; }

        constexpr void clear() const noexcept
        {
            for (std::size_t i = 0; i < numFrames_; ++i)
            {
                left_[i] = 0.0f;
                right_[i] = 0.0f;
            }
        }

        constexpr void addSample(std::size_t frame, float l, float r) const noexcept
        {
            left_[frame] += l;
            right_[frame] += r;
        }

    private:
        float* left_ = nullptr;
        float* right_ = nullptr;
        std::size_t numFrames_ = 0;
    };

} // namespace pw8::core
