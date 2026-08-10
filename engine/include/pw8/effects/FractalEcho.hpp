#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/DelayLine.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/effects/EffectTypes.hpp"

// FractalEcho -- this project's own invention, not modeled on any researched
// plugin. Full rationale in docs/FX_BANK.md "The invented algorithm: FractalEcho";
// summary here:
//
// Instead of a user-patched tree (ChowMatrix) or a single fixed algorithm, this
// effect PROCEDURALLY GENERATES a small self-similar delay tree from a 64-bit
// seed -- a `DeterministicRng`-driven fill of each node's delay time (scaled by a
// fixed ratio per tree depth, so deeper nodes have proportionally shorter,
// self-similar delay times, like a Cantor-set rhythm), feedback, pan, and level.
// Two such topologies (seedA, seedB) are generated ahead of time from two seeds.
//
// The actual novelty: a continuous `morph` parameter crossfades between the two
// topologies in COEFFICIENT SPACE, at audio rate, every sample -- not the usual
// signal-domain crossfade (running both networks and blending their output), and
// not a hard preset-to-preset snap. Every node's delay time/feedback/pan/level is
// itself a live lerp between topology A's and topology B's value for that node,
// so turning one knob continuously morphs the *rhythm and topology* of the delay
// network, not just its wet/dry balance or a filter over an otherwise-fixed
// structure. Because both topologies came from the same generator with the same
// bounded ranges, the interpolated coefficients are automatically stable and
// click-free at every point along the morph, with no envelope or crossfade timer
// needed to hide a structural discontinuity -- there isn't one.
namespace pw8::effects
{
    struct FractalNodeCoeffs
    {
        float delayMs = 0.0f;
        float feedback = 0.0f;
        float pan = 0.0f;
        float level = 0.0f;
    };

    class FractalEchoProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate;
            for (auto& line : lines_)
                line.prepare(sampleRate, kMaxTreeNodeDelaySeconds);
            reset();
            cachedSeedA_ = cachedSeedB_ = 0;
            cachedBaseMs_ = cachedRatio_ = cachedSpreadMs_ = -1.0f;
        }

        void reset() noexcept
        {
            for (auto& line : lines_)
                line.reset();
        }

        void processStereo(float inL, float inR, const EffectSlotParams& p, float& outL, float& outR) noexcept
        {
            regenerateTopologiesIfChanged(p);

            const float sr = static_cast<float>(sampleRate_);
            const float monoIn = (inL + inR) * 0.5f;
            const float morph = dsp::clamp(p.fractalMorph, 0.0f, 1.0f);

            std::array<float, kMaxDelayNodes> nodeOut{};
            float mixL = 0.0f;
            float mixR = 0.0f;

            for (std::size_t i = 0; i < kMaxDelayNodes; ++i)
            {
                const auto& a = topologyA_[i];
                const auto& b = topologyB_[i];
                const float delayMs = dsp::lerp(a.delayMs, b.delayMs, morph);
                const float feedback = dsp::lerp(a.feedback, b.feedback, morph);
                const float pan = dsp::lerp(a.pan, b.pan, morph);
                const float level = dsp::lerp(a.level, b.level, morph);

                const int parent = fractalParentOf(i);
                const float parentSignal =
                    (parent < 0) ? monoIn : nodeOut[static_cast<std::size_t>(parent)];

                const float delaySamples =
                    dsp::clamp(delayMs * 0.001f * sr, 1.0f, sr * kMaxTreeNodeDelaySeconds - 4.0f);
                const float delayed = lines_[i].readInterpolated(delaySamples);
                lines_[i].write(parentSignal + delayed * dsp::clamp(feedback, 0.0f, 0.95f));
                nodeOut[i] = delayed;

                const float panPos = dsp::clamp(pan, -1.0f, 1.0f) * 0.5f + 0.5f;
                const float halfPi = dsp::kPi * 0.5f;
                mixL += delayed * level * std::cos(panPos * halfPi);
                mixR += delayed * level * std::sin(panPos * halfPi);
            }

            const float mix = dsp::clamp(p.mix, 0.0f, 1.0f);
            outL = inL + mixL * mix;
            outR = inR + mixR * mix;
        }

        /// Exposed for testing: the currently generated topologies, without running
        /// any audio through them.
        [[nodiscard]] const std::array<FractalNodeCoeffs, kMaxDelayNodes>& topologyA() const noexcept
        {
            return topologyA_;
        }
        [[nodiscard]] const std::array<FractalNodeCoeffs, kMaxDelayNodes>& topologyB() const noexcept
        {
            return topologyB_;
        }
        void regenerateForTest(const EffectSlotParams& p) noexcept { regenerateTopologiesIfChanged(p); }

    private:
        /// Fixed, small, self-similar binary-tree connectivity: node 0 taps the
        /// slot's input directly; nodes 1-2 are its children; nodes 3-4 are node 1's
        /// children; node 5 is node 2's child. The GENERATOR fills in coefficients
        /// (delay/feedback/pan/level) per seed; the branching shape itself is fixed,
        /// which is what makes two differently-seeded topologies safely
        /// interpolable node-for-node (they always mean "the same branch").
        [[nodiscard]] static constexpr int fractalParentOf(std::size_t i) noexcept
        {
            switch (i)
            {
                case 0: return -1;
                case 1: return 0;
                case 2: return 0;
                case 3: return 1;
                case 4: return 1;
                case 5: return 2;
                default: return -1;
            }
        }

        [[nodiscard]] static constexpr int fractalDepthOf(std::size_t i) noexcept
        {
            switch (i)
            {
                case 0: return 0;
                case 1:
                case 2: return 1;
                case 3:
                case 4:
                case 5: return 2;
                default: return 0;
            }
        }

        static void generateTopology(std::uint64_t seed, float baseDelayMs, float ratio, float spreadMs,
                                      std::array<FractalNodeCoeffs, kMaxDelayNodes>& out) noexcept
        {
            dsp::DeterministicRng rng(seed);
            const float boundedRatio = dsp::clamp(ratio, 0.1f, 0.95f);
            for (std::size_t i = 0; i < kMaxDelayNodes; ++i)
            {
                const int depth = fractalDepthOf(i);
                const float scale = std::pow(boundedRatio, static_cast<float>(depth));
                out[i].delayMs = std::max(5.0f, baseDelayMs * scale + rng.nextRange(-spreadMs, spreadMs));
                out[i].feedback =
                    dsp::clamp(0.82f - static_cast<float>(depth) * 0.14f + rng.nextRange(-0.04f, 0.04f), 0.0f, 0.92f);
                out[i].pan = rng.nextRange(-1.0f, 1.0f);
                out[i].level = 1.0f / static_cast<float>(depth + 1);
            }
        }

        void regenerateTopologiesIfChanged(const EffectSlotParams& p) noexcept
        {
            // Cheap (kMaxDelayNodes RNG draws), but there's no reason to redo it every
            // sample -- only when the generator parameters actually move.
            if (p.fractalSeedA == cachedSeedA_ && p.fractalSeedB == cachedSeedB_ &&
                p.fractalBaseDelayMs == cachedBaseMs_ && p.fractalRatio == cachedRatio_ &&
                p.fractalSpreadMs == cachedSpreadMs_)
                return;

            generateTopology(p.fractalSeedA, p.fractalBaseDelayMs, p.fractalRatio, p.fractalSpreadMs, topologyA_);
            generateTopology(p.fractalSeedB, p.fractalBaseDelayMs, p.fractalRatio, p.fractalSpreadMs, topologyB_);

            cachedSeedA_ = p.fractalSeedA;
            cachedSeedB_ = p.fractalSeedB;
            cachedBaseMs_ = p.fractalBaseDelayMs;
            cachedRatio_ = p.fractalRatio;
            cachedSpreadMs_ = p.fractalSpreadMs;
        }

        double sampleRate_ = 48000.0;
        std::array<dsp::DelayLine, kMaxDelayNodes> lines_{};
        std::array<FractalNodeCoeffs, kMaxDelayNodes> topologyA_{};
        std::array<FractalNodeCoeffs, kMaxDelayNodes> topologyB_{};
        std::uint64_t cachedSeedA_ = 0;
        std::uint64_t cachedSeedB_ = 0;
        float cachedBaseMs_ = -1.0f;
        float cachedRatio_ = -1.0f;
        float cachedSpreadMs_ = -1.0f;
    };

} // namespace pw8::effects
