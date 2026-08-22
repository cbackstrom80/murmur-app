#pragma once

#include <vector>

#include "pw8/dsp/Math.hpp"

// A reusable mono delay line: linear-interpolated fractional-sample read,
// circular-buffer write. Used by every delay/echo-family effect in pw8/effects/.
//
// Realtime-safety note: `prepare()` is the ONLY place this class allocates
// (`std::vector::assign`), and it is documented control-thread-only, exactly the
// same pattern already used for `oscillator::WavetableTable::MipLevel::samples`
// (populated in `Engine::loadPatch()`, read-only on the audio thread thereafter).
// `write()`/`readInterpolated()` never resize the buffer and are audio-thread safe.
namespace pw8::dsp
{
    class DelayLine
    {
    public:
        /// Control-thread only. Sizes the buffer to hold at least `maxDelaySeconds`
        /// of audio at `sampleRate`, and clears it.
        void prepare(double sampleRate, float maxDelaySeconds) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            const std::size_t numSamples =
                static_cast<std::size_t>(sampleRate_ * static_cast<double>(std::max(maxDelaySeconds, 0.01f))) + 4;
            buffer_.assign(numSamples, 0.0f);
            writePos_ = 0;
        }

        /// Audio-thread safe. Zeroes the buffer without reallocating (e.g. on
        /// patch reload or a manual "clear delay tail" action).
        void reset() noexcept
        {
            std::fill(buffer_.begin(), buffer_.end(), 0.0f);
            writePos_ = 0;
        }

        /// Audio-thread safe. Reads `delaySamples` behind the current write
        /// position with linear interpolation. `delaySamples` is clamped to the
        /// buffer's actual capacity so a runaway modulated delay time can never
        /// read outside the allocated buffer.
        [[nodiscard]] float readInterpolated(float delaySamples) const noexcept
        {
            if (buffer_.empty())
                return 0.0f;

            const float maxDelay = static_cast<float>(buffer_.size()) - 2.0f;
            const float d = clamp(delaySamples, 0.0f, maxDelay);

            const float readPos = static_cast<float>(writePos_) - d;
            const float wrapped = wrapIndex(readPos);

            const std::size_t i0 = static_cast<std::size_t>(wrapped);
            const std::size_t i1 = (i0 + 1) % buffer_.size();
            const float frac = wrapped - static_cast<float>(i0);

            return lerp(buffer_[i0], buffer_[i1], frac);
        }

        /// Audio-thread safe. Reads `delaySamples` behind the current write
        /// position using 4-point Catmull-Rom (cubic Hermite) interpolation --
        /// smoother, lower-error than `readInterpolated()`'s linear
        /// interpolation for a continuously *modulated* read position (a fixed
        /// delay time gains little from this; a swept one -- an LFO'd comb
        /// line, a pitch shifter's grain tap -- is exactly where linear
        /// interpolation's error becomes audible). Needs one more sample of
        /// margin on each side than linear (`i-1` through `i+2` vs. linear's
        /// `i0`/`i0+1`), so the max-delay clamp is one sample tighter.
        ///
        /// A real, deliberate, additive opt-in: `readInterpolated()` itself is
        /// completely untouched, so every existing caller's output is
        /// byte-for-byte unaffected. Only call sites that explicitly switch to
        /// this method see any behavior change.
        [[nodiscard]] float readInterpolatedHermite(float delaySamples) const noexcept
        {
            if (buffer_.size() < 4)
                return 0.0f;

            const float maxDelay = static_cast<float>(buffer_.size()) - 3.0f;
            const float d = clamp(delaySamples, 0.0f, maxDelay);

            const float readPos = static_cast<float>(writePos_) - d;
            const float wrapped = wrapIndex(readPos);

            const std::size_t size = buffer_.size();
            const std::size_t i1 = static_cast<std::size_t>(wrapped);
            const std::size_t i0 = (i1 + size - 1) % size;
            const std::size_t i2 = (i1 + 1) % size;
            const std::size_t i3 = (i1 + 2) % size;
            const float t = wrapped - static_cast<float>(i1);

            const float y0 = buffer_[i0];
            const float y1 = buffer_[i1];
            const float y2 = buffer_[i2];
            const float y3 = buffer_[i3];

            // Catmull-Rom cubic Hermite spline (tension 0.5) -- the standard
            // real-time-audio choice: passes exactly through y1/y2, matched
            // first-derivative at both ends, no extra tuning parameter.
            const float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
            const float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            const float a2 = -0.5f * y0 + 0.5f * y2;
            const float a3 = y1;

            return ((a0 * t + a1) * t + a2) * t + a3;
        }

        /// Audio-thread safe. Writes one sample and advances the write head.
        void write(float sample) noexcept
        {
            if (buffer_.empty())
                return;
            buffer_[writePos_] = flushIfNotFinite(sample);
            writePos_ = (writePos_ + 1) % buffer_.size();
        }

        [[nodiscard]] std::size_t capacitySamples() const noexcept { return buffer_.size(); }

    private:
        [[nodiscard]] float wrapIndex(float idx) const noexcept
        {
            const float size = static_cast<float>(buffer_.size());
            while (idx < 0.0f)
                idx += size;
            while (idx >= size)
                idx -= size;
            return idx;
        }

        double sampleRate_ = 48000.0;
        std::vector<float> buffer_;
        std::size_t writePos_ = 0;
    };

} // namespace pw8::dsp
