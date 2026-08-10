#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// A lookahead peak limiter -- the master spec's "first effect set" basic limiter
// (docs/ROADMAP.md "GATE 10"). Unlike a plain fast-attack compressor at a high
// ratio (which can still let a very fast transient poke through before the
// envelope catches up), this guarantees no overshoot above the ceiling by
// construction: every incoming sample's own "gain needed to not exceed the
// ceiling" is recorded into a small ring buffer alongside the sample itself, and
// the gain actually applied to the OUTPUT (which reads `lookaheadMs` behind the
// input) is the minimum of every recorded gain within that lookahead window --
// so by the time a loud sample reaches the output tap, the gain has already had
// the full lookahead window to ramp down to what that sample needs.
//
// Deliberately the simple, provably-correct version: an O(window) scan per
// sample (`kMaxLimiterLookaheadSeconds` bounds the window to 20ms, ~1920 samples
// at 96kHz) rather than an O(1)-amortized monotonic-deque sliding-window-minimum.
// A real mastering-grade brickwall limiter would want the optimized version for a
// much longer lookahead; a synth's insert/master limiter doesn't need one.
namespace pw8::effects
{
    class LimiterProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            const auto capacity =
                static_cast<std::size_t>(sampleRate * static_cast<double>(kMaxLimiterLookaheadSeconds)) + 4;
            bufferL_.assign(capacity, 0.0f);
            bufferR_.assign(capacity, 0.0f);
            neededGain_.assign(capacity, 1.0f);
            reset();
        }

        void reset() noexcept
        {
            std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
            std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
            std::fill(neededGain_.begin(), neededGain_.end(), 1.0f);
            writePos_ = 0;
            smoothedGain_ = 1.0f;
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            if (bufferL_.empty())
            {
                outL = inL;
                outR = inR;
                return;
            }

            const float sr = static_cast<float>(sampleRate_);
            const auto lookaheadSamples = static_cast<std::size_t>(
                dsp::clamp(p.limiterLookaheadMs, 0.5f, kMaxLimiterLookaheadSeconds * 1000.0f) * 0.001f * sr);
            const std::size_t windowLen = std::min(lookaheadSamples + 1, bufferL_.size());

            const float ceilingLinear = dsp::dbToGain(p.limiterCeilingDb);
            const float peak = std::max(std::abs(inL), std::abs(inR));
            const float neededGain = peak > ceilingLinear ? ceilingLinear / peak : 1.0f;

            bufferL_[writePos_] = inL;
            bufferR_[writePos_] = inR;
            neededGain_[writePos_] = neededGain;

            float rawGain = 1.0f;
            std::size_t scanIdx = writePos_;
            for (std::size_t i = 0; i < windowLen; ++i)
            {
                rawGain = std::min(rawGain, neededGain_[scanIdx]);
                scanIdx = (scanIdx == 0) ? (neededGain_.size() - 1) : (scanIdx - 1);
            }

            if (rawGain < smoothedGain_)
            {
                // The lookahead window already had time to see this coming -- catch
                // down immediately rather than adding a second smoothing delay on
                // top of the lookahead itself.
                smoothedGain_ = rawGain;
            }
            else
            {
                const float releaseCoeff =
                    1.0f - std::exp(-1.0f / (0.001f * std::max(p.limiterReleaseMs, 1.0f) * sr));
                smoothedGain_ += (rawGain - smoothedGain_) * releaseCoeff;
            }

            const std::size_t readIdx = (writePos_ + bufferL_.size() - lookaheadSamples) % bufferL_.size();
            const float delayedL = bufferL_[readIdx];
            const float delayedR = bufferR_[readIdx];

            writePos_ = (writePos_ + 1) % bufferL_.size();

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = dsp::lerp(delayedL, delayedL * smoothedGain_, mix);
            outR = dsp::lerp(delayedR, delayedR * smoothedGain_, mix);
        }

    private:
        double sampleRate_ = 48000.0;
        std::vector<float> bufferL_;
        std::vector<float> bufferR_;
        std::vector<float> neededGain_;
        std::size_t writePos_ = 0;
        float smoothedGain_ = 1.0f;
    };

} // namespace pw8::effects
