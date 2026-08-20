#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/oscillator/WavetableOscillator.hpp"

// Engine Type 6 -- Granular.
//
// Grains are read from the SAME `WavetableTable` data an operator's
// `wavetableId` already points at (reusing the entire existing table-loading
// pipeline -- no new sample format/loader needed), treated as one flat,
// continuous sample buffer (`numFrames * samplesPerFrame` samples,
// frame-major) rather than the small per-cycle frames the Wavetable engine's
// phase-based synthesis reads them as. `wavetableFramePosition` (already a
// real automatable parameter, previously Wavetable-engine-only) is reused
// verbatim as each grain's base read position, 0..1 across that flat buffer
// -- a deliberate, documented reuse, not a coincidence (docs/DSP_ENGINE.md's
// original Granular scope called this out explicitly).
//
// Fixed-capacity grain pool (kMaxGrains, a plain array member -- allocated
// once at construction, zero realtime allocation, per docs/DSP_ENGINE.md's
// explicit requirement). Each grain is a simple {read position, playback
// position, length, pitch ratio} struct, Hann-windowed, triggered at
// `densityHz` via the same phase-accumulator retarget-clock technique
// noise::NoiseSource's Sample & Hold variant uses. New grains steal pool
// slots round-robin (not "oldest first") -- simpler bookkeeping, and at any
// sane density/grain-size combination the pool cycles faster than any single
// grain's natural lifetime overlaps meaningfully with reuse.

namespace pw8::oscillator
{
    struct GranularParams
    {
        /// Base grain read position within the flattened source buffer, 0..1 --
        /// reuses the exact same field/semantics as the Wavetable engine's own
        /// frame-position parameter.
        float framePosition01 = 0.0f;
        /// Grain trigger rate, 0.5-200 Hz.
        float densityHz = 20.0f;
        /// Grain playback duration, 1-500 ms.
        float grainSizeMs = 60.0f;
        /// Random read-position jitter per grain, 0..1 (fraction of the whole
        /// source buffer's length).
        float positionJitter = 0.1f;
        /// Random pitch (playback-rate) jitter per grain, 0..1, scaled up to
        /// +-12 semitones at 1.0.
        float pitchJitter = 0.0f;
    };

    class GranularOscillator
    {
    public:
        static constexpr int kMaxGrains = 12;

        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            // Force hannTable()'s lazy static-local init to happen here, off the audio
            // thread (prepare() is guaranteed non-concurrent with processBlock/renderSample
            // -- see MurmurProcessor.cpp), rather than on whichever thread happens to
            // render the very first grain. C++11 static-local init is already
            // thread-safe/one-time on its own, and per-Engine audio processing is
            // single-threaded anyway, so this isn't fixing a real race -- it's just
            // making "the table build cost never happens mid-render" explicit and
            // provable rather than relying on that reasoning implicitly.
            (void)hann(0.0f);
        }

        /// `seed` should come from dsp::DeterministicRng::deriveSeed() at note-on
        /// (see Voice::noteOn / OperatorState::seedGranular()) -- never a
        /// free-running/time-based seed.
        void reset(std::uint64_t seed) noexcept
        {
            rng_.reseed(seed);
            clockPhase_ = 0.0f;
            nextGrainSlot_ = 0;
            for (auto& g : grains_)
                g.active = false;
        }

        void setFrequency(float hz) noexcept { frequencyHz_ = hz; }

        [[nodiscard]] float renderSample(const GranularParams& params, const WavetableView& source) noexcept
        {
            if (!source.isValid())
                return 0.0f;

            const float densityHz = dsp::clamp(params.densityHz, 0.1f, 500.0f);
            clockPhase_ += densityHz / static_cast<float>(sampleRate_);
            if (clockPhase_ >= 1.0f)
            {
                clockPhase_ -= std::floor(clockPhase_);
                triggerGrain(params, source);
            }

            const auto totalSamples =
                static_cast<std::size_t>(source.numFrames) * static_cast<std::size_t>(source.samplesPerFrame);

            float sum = 0.0f;
            int activeCount = 0;
            for (auto& g : grains_)
            {
                if (!g.active)
                    continue;
                ++activeCount;

                const float window = hann(g.playPos / g.lengthSamples);
                sum += readInterpolated(source, totalSamples, g.readPos) * window;

                g.readPos += g.pitchRatio;
                g.playPos += 1.0f;
                if (g.playPos >= g.lengthSamples || g.readPos < 0.0f ||
                    g.readPos >= static_cast<float>(totalSamples) - 1.0f)
                    g.active = false;
            }

            // Energy-normalize by the number of currently-overlapping grains --
            // decorrelated overlapping grains sum roughly as sqrt(N) in amplitude,
            // not N, so this keeps overall loudness roughly independent of density
            // rather than getting louder every time more grains happen to overlap.
            const float norm = activeCount > 0 ? 1.0f / std::sqrt(static_cast<float>(activeCount)) : 0.0f;
            return dsp::flushIfNotFinite(sum * norm);
        }

    private:
        struct Grain
        {
            bool active = false;
            float readPos = 0.0f;
            float playPos = 0.0f;
            float lengthSamples = 1.0f;
            float pitchRatio = 1.0f;
        };

        void triggerGrain(const GranularParams& params, const WavetableView& source) noexcept
        {
            const auto totalSamples =
                static_cast<std::size_t>(source.numFrames) * static_cast<std::size_t>(source.samplesPerFrame);
            if (totalSamples < 4)
                return;

            auto& g = grains_[static_cast<std::size_t>(nextGrainSlot_)];
            nextGrainSlot_ = (nextGrainSlot_ + 1) % kMaxGrains;

            const float totalF = static_cast<float>(totalSamples);
            const float baseReadPos = dsp::clamp(params.framePosition01, 0.0f, 1.0f) * (totalF - 1.0f);
            const float jitterRange = dsp::clamp(params.positionJitter, 0.0f, 1.0f) * totalF;
            g.readPos = dsp::clamp(baseReadPos + rng_.nextRange(-0.5f, 0.5f) * jitterRange, 0.0f, totalF - 2.0f);

            const float grainSizeMs = dsp::clamp(params.grainSizeMs, 1.0f, 2000.0f);
            g.lengthSamples = std::max((grainSizeMs * 0.001f) * static_cast<float>(sampleRate_), 4.0f);
            g.playPos = 0.0f;

            // Playback rate is keyed off the operator's carrier frequency against a
            // fixed C4 "root key" -- the standard sampler/granular convention for
            // making a grain cloud actually playable as a pitched synth voice, not
            // just an unpitched texture generator -- with per-grain semitone jitter
            // layered on top.
            constexpr float kGranularRootHz = 261.6256f; // C4.
            constexpr float kMaxPitchJitterSemitones = 12.0f;
            const float pitchJitter = dsp::clamp(params.pitchJitter, 0.0f, 1.0f);
            const float jitterSemitones = rng_.nextRange(-1.0f, 1.0f) * pitchJitter * kMaxPitchJitterSemitones;
            const float baseRatio = frequencyHz_ / kGranularRootHz;
            g.pitchRatio = baseRatio * std::pow(2.0f, jitterSemitones / 12.0f);

            g.active = true;
        }

        // hann(t) is called once per active grain per sample (up to kMaxGrains/sample)
        // -- a precomputed, linearly-interpolated table replaces the direct std::cos
        // call. 512 entries is comfortably more resolution than a linear interpolant
        // needs for a function this smooth; see tests/dsp/GranularOscillatorTests.cpp
        // for the tolerance-checked comparison against the direct formula.
        static constexpr int kHannTableSize = 512;

        [[nodiscard]] static const std::array<float, kHannTableSize + 1>& hannTable() noexcept
        {
            static const std::array<float, kHannTableSize + 1> table = [] {
                std::array<float, kHannTableSize + 1> t{};
                for (int i = 0; i <= kHannTableSize; ++i)
                {
                    const float frac = static_cast<float>(i) / static_cast<float>(kHannTableSize);
                    t[static_cast<std::size_t>(i)] = 0.5f - 0.5f * std::cos(dsp::kTwoPi * frac);
                }
                return t;
            }();
            return table;
        }

        [[nodiscard]] static float hann(float t) noexcept
        {
            const float tc = dsp::clamp(t, 0.0f, 1.0f);
            const float pos = tc * static_cast<float>(kHannTableSize);
            const auto i0 = static_cast<std::size_t>(pos);
            const std::size_t i1 = std::min(i0 + 1, static_cast<std::size_t>(kHannTableSize));
            const float frac = pos - static_cast<float>(i0);
            const auto& table = hannTable();
            return dsp::lerp(table[i0], table[i1], frac);
        }

        [[nodiscard]] static float flatSampleAt(const WavetableView& source, std::size_t flatIndex) noexcept
        {
            const auto samplesPerFrame = static_cast<std::size_t>(source.samplesPerFrame);
            const int frame = static_cast<int>(flatIndex / samplesPerFrame);
            const int index = static_cast<int>(flatIndex % samplesPerFrame);
            return source.sampleAt(frame, index);
        }

        [[nodiscard]] static float readInterpolated(const WavetableView& source, std::size_t totalSamples,
                                                      float pos) noexcept
        {
            pos = std::max(pos, 0.0f);
            const std::size_t maxIdx = totalSamples > 0 ? totalSamples - 1 : 0;
            std::size_t idx0 = static_cast<std::size_t>(pos);
            if (idx0 >= maxIdx)
                idx0 = maxIdx > 0 ? maxIdx - 1 : 0;
            const std::size_t idx1 = idx0 + 1;
            const float frac = pos - static_cast<float>(idx0);
            return dsp::lerp(flatSampleAt(source, idx0), flatSampleAt(source, idx1), frac);
        }

        double sampleRate_ = 48000.0;
        float frequencyHz_ = 440.0f;

        dsp::DeterministicRng rng_;
        float clockPhase_ = 0.0f;
        int nextGrainSlot_ = 0;

        std::array<Grain, kMaxGrains> grains_{};
    };

} // namespace pw8::oscillator
