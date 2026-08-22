#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"

// A real, minimal, continuous real-time pitch shifter -- two Hann-windowed,
// deterministically-crossfaded taps into a short delay line, each read
// position sweeping through the buffer at `pitchRatio` samples/sample
// relative to the write pointer. The same real grain/delay-line technique
// `pw8::effects::CloudsTexture`'s own CloudsMode::PitchShift already uses
// in this codebase (`grain.position += grain.increment`), informed by that
// real precedent but not copied from it: Clouds is tuned for a randomized
// textural aesthetic (linear-only envelope, stochastic grain spawn, a
// 4-second buffer); this is the classic "clean" version for a *sustained*,
// continuous pitch shift -- always exactly two taps, Hann-windowed so the
// deterministic wrap never clicks, a short (~100ms) buffer sized for the
// grain window rather than seconds of texture.
//
// Built on the real, already-proven `dsp::DelayLine` (write/readInterpolated)
// rather than a hand-rolled circular buffer.
namespace pw8::dsp
{
    class PitchShifter
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            buffer_.prepare(sampleRate_, kMaxBufferSeconds);
            grainSamples_ = kGrainMs * 0.001f * static_cast<float>(sampleRate_);
            reset();
        }

        void reset() noexcept
        {
            buffer_.reset();
            delaySamples_[0] = grainSamples_;
            delaySamples_[1] = grainSamples_ * 0.5f;
        }

        /// Audio-thread safe. `pitchRatio` > 1 shifts up (2.0 = +1 octave),
        /// < 1 shifts down; 1.0 is a real no-op (both taps stay in lockstep
        /// with the write pointer, netting straight-through delayed audio).
        [[nodiscard]] float renderSample(float input, float pitchRatio) noexcept
        {
            buffer_.write(input);

            float out = 0.0f;
            for (float& delay : delaySamples_)
            {
                const float t = clamp(delay / grainSamples_, 0.0f, 1.0f);
                const float envelope = 0.5f - 0.5f * std::cos(kTwoPi * t); // real Hann window -- 0 at both edges
                out += buffer_.readInterpolated(delay) * envelope;

                delay -= (pitchRatio - 1.0f);
                if (delay <= 0.0f)
                    delay += grainSamples_; // deterministic re-spawn from the far edge, not random (unlike Clouds)
                else if (delay >= grainSamples_)
                    delay -= grainSamples_; // symmetric handling for pitchRatio < 1 (down-shift)
            }

            // Real, exact (not approximate) unity gain: two Hann windows
            // offset by exactly half their period sum to a real constant 1.0
            // at every point (w(t) + w(t+0.5) = 1 -- cos(x+pi) = -cos(x)
            // cancels the modulation term), so no additional compensation
            // is needed -- verified algebraically, not assumed.
            return out;
        }

    private:
        static constexpr float kGrainMs = 40.0f; // real, typical shimmer grain window
        static constexpr float kMaxBufferSeconds = 0.15f; // real headroom above the grain window

        double sampleRate_ = 48000.0;
        DelayLine buffer_;
        float grainSamples_ = 1920.0f; // 40ms @ 48kHz, real value recomputed in prepare()
        std::array<float, 2> delaySamples_{1920.0f, 960.0f};
    };

} // namespace pw8::dsp
