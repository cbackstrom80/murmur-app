#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/operator/OperatorNode.hpp"
#include "pw8/oscillator/ResonatorOscillator.hpp"

using namespace pw8;
using namespace pw8::oscillator;

namespace
{
    std::vector<float> renderResonator(float freqHz, const ResonatorParams& params, std::uint64_t seed,
                                        double sampleRate, std::size_t numSamples)
    {
        ResonatorOscillator osc;
        osc.prepare(sampleRate);
        osc.reset(seed);
        osc.setFrequency(freqHz);

        std::vector<float> out(numSamples);
        for (std::size_t i = 0; i < numSamples; ++i)
            out[i] = osc.renderSample(params);
        return out;
    }

    float rms(const std::vector<float>& samples, std::size_t start, std::size_t count)
    {
        double sumSq = 0.0;
        for (std::size_t i = start; i < start + count; ++i)
            sumSq += static_cast<double>(samples[i]) * samples[i];
        return static_cast<float>(std::sqrt(sumSq / static_cast<double>(count)));
    }

    // Sum of squared FFT magnitude at a specific harmonic bin, from a windowed
    // slice of a larger sample buffer (the window need not start at sample 0).
    double energyAtBin(const std::vector<float>& samples, std::size_t windowStart, std::size_t n, int bin)
    {
        std::vector<std::complex<float>> data(n);
        for (std::size_t i = 0; i < n; ++i)
            data[i] = std::complex<float>(samples[windowStart + i], 0.0f);
        dsp::fft(data, false);
        return std::norm(data[static_cast<std::size_t>(bin)]);
    }
} // namespace

TEST_CASE("ResonatorOscillator output stays bounded and finite across the extreme parameter matrix",
          "[resonator][stability]")
{
    const double sampleRate = 48000.0;
    for (int count : {2, 5, 8})
        for (float structure : {0.0f, 1.0f})
            for (float decay : {0.0f, 1.0f})
                for (float damping : {0.0f, 1.0f})
                    for (float brightness : {0.0f, 1.0f})
                        for (float freqHz : {20.0f, 440.0f, 8000.0f})
                        {
                            ResonatorParams params;
                            params.modeCount = count;
                            params.structure = structure;
                            params.decay = decay;
                            params.damping = damping;
                            params.brightness = brightness;

                            const auto samples = renderResonator(freqHz, params, 0xABCDEFULL, sampleRate, 4096);
                            for (float s : samples)
                            {
                                REQUIRE(std::isfinite(s));
                                REQUIRE(std::abs(s) <= 8.0f);
                            }
                        }
}

TEST_CASE("ResonatorOscillator: same seed reproduces the exact same output stream", "[resonator][determinism]")
{
    ResonatorParams params;
    const auto a = renderResonator(220.0f, params, 0x1234ULL, 48000.0, 4096);
    const auto b = renderResonator(220.0f, params, 0x1234ULL, 48000.0, 4096);
    REQUIRE(a == b);

    const auto c = renderResonator(220.0f, params, 0x5678ULL, 48000.0, 4096);
    REQUIRE(a != c);
}

TEST_CASE("ResonatorOscillator: output measurably rings out (decays) after the exciter burst ends",
          "[resonator][measured]")
{
    ResonatorParams params;
    params.decay = 0.7f;
    params.modeCount = 6;

    // 1 second at 48kHz -- the ~6ms exciter burst is long gone well before the
    // "late" window below.
    const auto samples = renderResonator(220.0f, params, 0xF00DULL, 48000.0, 48000);

    const float earlyRms = rms(samples, 0, 2048);       // right after the burst.
    const float lateRms = rms(samples, 40000, 2048);    // ~0.83s in.

    REQUIRE(earlyRms > 0.0f);
    REQUIRE(lateRms < earlyRms * 0.3f); // measurably rung down, not just "different".
}

TEST_CASE("ResonatorOscillator: resonatorDamping measurably shortens a higher mode's decay relative to the fundamental's",
          "[resonator][measured]")
{
    // structure=0 keeps every mode at an exact harmonic multiple, so both the
    // fundamental and mode 6's frequency land on exact FFT bins.
    constexpr std::size_t n = 8192;
    constexpr float freqHz = 40.0f * 48000.0f / static_cast<float>(n); // bin 40.
    constexpr int fundamentalBin = 40;
    constexpr int mode6Bin = 40 * 6;

    ResonatorParams lowDamping;
    lowDamping.structure = 0.0f;
    lowDamping.decay = 0.6f;
    lowDamping.damping = 0.0f;
    lowDamping.brightness = 1.0f;
    lowDamping.modeCount = 6;

    ResonatorParams highDamping = lowDamping;
    highDamping.damping = 1.0f;

    constexpr std::size_t numSamples = 32768;
    const auto lowDampingSamples = renderResonator(freqHz, lowDamping, 0xAAAAULL, 48000.0, numSamples);
    const auto highDampingSamples = renderResonator(freqHz, highDamping, 0xAAAAULL, 48000.0, numSamples);

    constexpr std::size_t lateWindowStart = 16384;

    auto ratioAt = [&](const std::vector<float>& samples, std::size_t windowStart) {
        const double fundamental = energyAtBin(samples, windowStart, n, fundamentalBin);
        const double mode6 = energyAtBin(samples, windowStart, n, mode6Bin);
        return mode6 / std::max(fundamental, 1.0e-9);
    };

    const double lowDampingEarlyRatio = ratioAt(lowDampingSamples, 0);
    const double lowDampingLateRatio = ratioAt(lowDampingSamples, lateWindowStart);
    const double highDampingEarlyRatio = ratioAt(highDampingSamples, 0);
    const double highDampingLateRatio = ratioAt(highDampingSamples, lateWindowStart);

    // Lower Q (higher damping) also changes each mode's steady-state gain to a
    // short burst, not just its decay rate, so comparing late-window ratios
    // directly conflates "decayed faster" with "started from a different level."
    // Instead, compare each config's own late/early ratio-of-ratios (how much
    // mode 6's share of the total energy shrank over time, self-normalized
    // against that config's own starting balance) -- decayFactor < 1 means mode
    // 6 lost ground to the fundamental; the smaller it is, the faster mode 6
    // decayed relative to the fundamental.
    const double lowDampingDecayFactor = lowDampingLateRatio / lowDampingEarlyRatio;
    const double highDampingDecayFactor = highDampingLateRatio / highDampingEarlyRatio;

    REQUIRE(highDampingDecayFactor < lowDampingDecayFactor);
}

TEST_CASE("OperatorState renders Resonator through the full operator path", "[resonator][operator]")
{
    op::OperatorState state;
    state.prepare(48000.0);
    state.seedResonator(dsp::DeterministicRng::deriveSeed(42, 0, 0));

    op::OperatorParams params;
    params.engine = algorithm::EngineType::Resonator;
    params.frequencyRatio = 1.0f;
    params.level = 1.0f;
    params.resonatorStructure = 0.3f;
    params.resonatorDecay = 0.5f;
    params.resonatorDamping = 0.5f;
    params.resonatorBrightness = 0.5f;
    params.resonatorModeCount = 6.0f;

    bool sawNonZero = false;
    for (int i = 0; i < 8192; ++i)
    {
        const float s = state.render(params, nullptr, 220.0f, 0.0f, 0.0f);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 8.0f);
        if (s != 0.0f)
            sawNonZero = true;
    }
    REQUIRE(sawNonZero);
}
