#pragma once

#include <cmath>

namespace pw8::dsp
{
    /// One-pole exponential smoother for continuous realtime parameters
    /// (gain, pan, cutoff, morph, ...). Avoids zipper noise without the cost
    /// of a full sample-accurate ramp. Safe to call from the audio thread;
    /// `setTimeMs` should only be called from control-rate code (once per block).
    class OnePoleSmoother
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            recalculateCoefficient();
        }

        void setTimeMs(float timeMs) noexcept
        {
            timeMs_ = timeMs;
            recalculateCoefficient();
        }

        /// Jump immediately to a value with no smoothing (e.g. on voice start).
        void snapTo(float value) noexcept
        {
            current_ = value;
            target_ = value;
        }

        void setTarget(float value) noexcept { target_ = value; }

        [[nodiscard]] float getNext() noexcept
        {
            current_ += (target_ - current_) * coefficient_;
            return current_;
        }

        [[nodiscard]] float getCurrentValue() const noexcept { return current_; }
        [[nodiscard]] float getTargetValue() const noexcept { return target_; }

        [[nodiscard]] bool isSmoothing() const noexcept
        {
            return std::abs(target_ - current_) > 1.0e-6f;
        }

    private:
        void recalculateCoefficient() noexcept
        {
            if (sampleRate_ <= 0.0 || timeMs_ <= 0.0f)
            {
                coefficient_ = 1.0f;
                return;
            }
            const double timeSeconds = static_cast<double>(timeMs_) * 0.001;
            // 1 - exp(-1 / (timeSeconds * sampleRate)) : standard one-pole time-constant coefficient.
            coefficient_ = static_cast<float>(1.0 - std::exp(-1.0 / (timeSeconds * sampleRate_)));
        }

        double sampleRate_ = 48000.0;
        float timeMs_ = 10.0f;
        float coefficient_ = 1.0f;
        float current_ = 0.0f;
        float target_ = 0.0f;
    };

    /// Simple block-rate linear ramp, useful where a per-sample linear slope is
    /// preferable to exponential settling (e.g. short crossfades during voice stealing).
    class LinearRamp
    {
    public:
        void setLengthSamples(int numSamples) noexcept
        {
            lengthSamples_ = numSamples > 0 ? numSamples : 1;
        }

        void startFromCurrent(float from, float to) noexcept
        {
            current_ = from;
            target_ = to;
            stepIndex_ = 0;
            increment_ = (target_ - current_) / static_cast<float>(lengthSamples_);
        }

        [[nodiscard]] float getNext() noexcept
        {
            if (stepIndex_ >= lengthSamples_)
                return target_;
            current_ += increment_;
            ++stepIndex_;
            return current_;
        }

        [[nodiscard]] bool isFinished() const noexcept { return stepIndex_ >= lengthSamples_; }

    private:
        int lengthSamples_ = 1;
        int stepIndex_ = 0;
        float current_ = 0.0f;
        float target_ = 0.0f;
        float increment_ = 0.0f;
    };

} // namespace pw8::dsp
