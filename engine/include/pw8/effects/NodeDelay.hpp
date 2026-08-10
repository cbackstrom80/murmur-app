#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/effects/EffectTypes.hpp"

// A fixed-capacity tree of delay nodes, informed by ChowMatrix's "infinitely
// growable tree of delay lines" concept (docs/FX_BANK.md "ChowMatrix") -- adapted
// to this codebase's realtime-safety discipline as a bounded (`kMaxDelayNodes`)
// structure rather than a runtime-growable one, the same fixed-capacity-over-
// dynamic-allocation choice made everywhere else in pw8_core.
//
// Each node reads `EffectSlotParams::nodes[i].parentIndex` as the index of the
// node whose *output* feeds this node's *input* (or -1 for "fed directly from the
// slot's input"). Requiring `parentIndex < i` (enforced by
// `PatchSerializer::fromJson`, defended again here) makes the tree acyclic by
// construction and lets a single forward pass over the fixed node array process
// the whole tree with no recursion and no separate topological sort -- the same
// "always route from a lower index" trick `AlgorithmGraphCompiler` and
// `pw8-fuzz-render`'s patch generator already rely on.
namespace pw8::effects
{
    class NodeDelayProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            for (auto& line : lines_)
                line.prepare(sampleRate, kMaxTreeNodeDelaySeconds);
            reset();
            rng_.reseed(0x9E3779B97F4A7C15ULL);
        }

        void reset() noexcept
        {
            for (auto& line : lines_)
                line.reset();
            wanderPhase_.fill(0.0f);
            wanderTarget_.fill(0.0f);
            wanderCurrent_.fill(0.0f);
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            const float sr = static_cast<float>(sampleRate_);
            const float monoIn = (inL + inR) * 0.5f;

            std::array<float, kMaxDelayNodes> nodeOut{};
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (std::size_t i = 0; i < kMaxDelayNodes; ++i)
            {
                const auto& node = p.nodes[i];
                const float parentSignal = (node.parentIndex < 0 || node.parentIndex >= static_cast<int>(i))
                                                ? monoIn
                                                : nodeOut[static_cast<std::size_t>(node.parentIndex)];

                // "Insanity": a slow, smoothed, deterministic random walk on each
                // node's delay time, scaled by `nodeInsanity`. Same target-then-glide
                // shape as LFO SampleHold, so it stays click-free even at high wander.
                wanderPhase_[i] += kWanderRateHz / sr;
                if (wanderPhase_[i] >= 1.0f)
                {
                    wanderPhase_[i] -= 1.0f;
                    wanderTarget_[i] = rng_.nextRange(-1.0f, 1.0f);
                }
                wanderCurrent_[i] += (wanderTarget_[i] - wanderCurrent_[i]) * kWanderGlideCoeff;
                const float wanderMs = wanderCurrent_[i] * dsp::clamp(p.nodeInsanity, 0.0f, 1.0f) * kMaxWanderMs;

                const float delaySamples = dsp::clamp((node.delayMs + wanderMs) * 0.001f * sr, 1.0f,
                                                        sr * kMaxTreeNodeDelaySeconds - 4.0f);

                const float delayed = lines_[i].readInterpolated(delaySamples);
                const float distorted = dsp::softSaturate(delayed, 1.0f + dsp::clamp(node.distortion, 0.0f, 1.0f) * 8.0f);
                const float feedback = dsp::clamp(node.feedback, 0.0f, 0.95f);
                lines_[i].write(parentSignal + distorted * feedback);

                nodeOut[i] = distorted;

                if (node.enabled)
                {
                    const float gain = dsp::clamp(node.level, 0.0f, 1.0f);
                    const float panPos = dsp::clamp(node.pan, -1.0f, 1.0f) * 0.5f + 0.5f; // 0..1
                    const float halfPi = dsp::kPi * 0.5f;
                    mixL += distorted * gain * std::cos(panPos * halfPi);
                    mixR += distorted * gain * std::sin(panPos * halfPi);
                }
            }

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = inL + mixL * mix;
            outR = inR + mixR * mix;
        }

    private:
        static constexpr float kWanderRateHz = 0.7f;
        static constexpr float kWanderGlideCoeff = 0.01f;
        static constexpr float kMaxWanderMs = 40.0f;

        double sampleRate_ = 48000.0;
        std::array<dsp::DelayLine, kMaxDelayNodes> lines_{};
        std::array<float, kMaxDelayNodes> wanderPhase_{};
        std::array<float, kMaxDelayNodes> wanderTarget_{};
        std::array<float, kMaxDelayNodes> wanderCurrent_{};
        dsp::DeterministicRng rng_{};
    };

} // namespace pw8::effects
