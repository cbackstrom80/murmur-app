#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/effects/EffectTypes.hpp"

// A compact 4-line Feedback Delay Network (FDN) reverb -- the master spec's
// "first effect set" basic reverb (docs/ROADMAP.md "GATE 10"). Technique: Jean-Marc
// Jot-style FDN with a Householder reflection as the feedback mixing matrix -- a
// well-documented, openly-published family of algorithmic reverb designs (the same
// citation category as this codebase's other DSP blocks), not modeled on any
// specific product.
//
// The Householder matrix for N lines is `I - (2/N)*ones(N,N)`: applying it to a
// vector of the N lines' current outputs is just `mixed[i] = lineOut[i] -
// (2/N)*sum(lineOut)` -- O(N), no explicit matrix multiply needed -- and it's
// unitary (energy-preserving) regardless of N, which is what keeps the network's
// total energy bounded without per-line gain bookkeeping. The 4 delay lines use
// mutually-irrational-ish base lengths (chosen with no small common integer
// ratios) to avoid the metallic, comb-filtered ringing equal/rational-ratio delay
// times produce.
namespace pw8::effects
{
    class ReverbProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            preDelay_.prepare(sampleRate, kMaxReverbPreDelaySeconds);
            for (auto& line : lines_)
                line.prepare(sampleRate, kMaxReverbLineSeconds);
            reset();
        }

        void reset() noexcept
        {
            preDelay_.reset();
            for (auto& line : lines_)
                line.reset();
            dampingStates_.fill(0.0f);
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float mono = (inL + inR) * 0.5f;
            const float sizeScale = dsp::clamp(p.reverbSizeParam, 0.2f, 3.0f);

            // Floored at 1 sample, not 0: `dsp::DelayLine` reads before writing (the
            // same order every other delay-based effect in this codebase uses), so a
            // delay of exactly 0 samples reads whatever was written a full buffer
            // length ago (near-silence at startup) instead of "this sample,
            // undelayed" -- a real bug caught by this effect's own render-level test
            // showing total silence for ~200ms where a short pre-delay tail was
            // expected. `reverbPreDelayMs` can still be set to 0 in a patch; it just
            // means "as close to zero pre-delay as a delay line can express."
            const float preDelaySamples =
                dsp::clamp(p.reverbPreDelayMs * 0.001f * sr, 1.0f, sr * kMaxReverbPreDelaySeconds - 4.0f);
            const float predelayed = preDelay_.readInterpolated(preDelaySamples);
            preDelay_.write(mono);

            std::array<float, kNumReverbLines> lineOut{};
            std::array<float, kNumReverbLines> delaySamples{};
            for (std::size_t i = 0; i < kNumReverbLines; ++i)
            {
                delaySamples[i] = dsp::clamp(kBaseDelayMs[i] * sizeScale * 0.001f * sr, 1.0f,
                                              sr * kMaxReverbLineSeconds - 4.0f);
                lineOut[i] = lines_[i].readInterpolated(delaySamples[i]);
            }

            float sum = 0.0f;
            for (const float v : lineOut)
                sum += v;
            const float householderFactor = 2.0f / static_cast<float>(kNumReverbLines);

            const float dampCoeff =
                std::exp(-dsp::kTwoPi * dsp::clamp(p.reverbDampingHz, 200.0f, sr * 0.49f) / sr);
            const float decaySecondsParam = std::max(p.reverbDecaySeconds, 0.05f);

            float wetL = 0.0f;
            float wetR = 0.0f;
            for (std::size_t i = 0; i < kNumReverbLines; ++i)
            {
                const float mixed = lineOut[i] - householderFactor * sum; // Householder reflection.

                dampingStates_[i] = dampCoeff * dampingStates_[i] + (1.0f - dampCoeff) * mixed;
                const float damped = dampingStates_[i];

                // RT60-derived per-line decay gain: -60dB after decaySecondsParam,
                // scaled by how much of that time one round trip through this line
                // (delaySamples[i]/sr) actually represents.
                const float lineSeconds = delaySamples[i] / sr;
                const float decayGain = std::pow(10.0f, -3.0f * lineSeconds / decaySecondsParam);

                lines_[i].write(predelayed * 0.5f + damped * decayGain);

                // Tap the raw (pre-Householder) line outputs for stereo output --
                // the feedback matrix still diffuses the energy that feeds back in,
                // but the direct taps stay slightly decorrelated per line, which is
                // what gives the stereo field width rather than a mono reverb
                // panned by a single gain.
                if (i % 2 == 0)
                    wetL += lineOut[i] * 0.5f;
                else
                    wetR += lineOut[i] * 0.5f;
            }

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = inL + wetL * mix;
            outR = inR + wetR * mix;
        }

    private:
        static constexpr std::array<float, kNumReverbLines> kBaseDelayMs = {29.7f, 37.1f, 41.3f, 47.9f};

        double sampleRate_ = 48000.0;
        dsp::DelayLine preDelay_;
        std::array<dsp::DelayLine, kNumReverbLines> lines_{};
        std::array<float, kNumReverbLines> dampingStates_{};
    };

} // namespace pw8::effects
