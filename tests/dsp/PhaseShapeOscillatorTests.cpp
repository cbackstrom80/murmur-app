#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/operator/OperatorNode.hpp"
#include "pw8/oscillator/PhaseShapeOscillator.hpp"

using namespace pw8;
using namespace pw8::oscillator;

namespace
{
    std::vector<float> renderPhaseShape(float freqHz, const PhaseShapeParams& params, double sampleRate,
                                         std::size_t warmupSamples, std::size_t numSamples)
    {
        PhaseShapeOscillator osc;
        osc.prepare(sampleRate);
        osc.reset();
        osc.setFrequency(freqHz);

        for (std::size_t i = 0; i < warmupSamples; ++i)
            static_cast<void>(osc.renderSample(params));

        std::vector<float> out(numSamples);
        for (std::size_t i = 0; i < numSamples; ++i)
            out[i] = osc.renderSample(params);
        return out;
    }

    /// Sum of squared FFT magnitude across every bin except `fundamentalBin`,
    /// divided by the fundamental bin's own energy -- a measure of how much
    /// harmonic/distortion content exists relative to the fundamental.
    double nonFundamentalEnergyRatio(const std::vector<float>& samples, std::size_t fundamentalBin)
    {
        const std::size_t n = samples.size();
        std::vector<std::complex<float>> data(n);
        for (std::size_t i = 0; i < n; ++i)
            data[i] = std::complex<float>(samples[i], 0.0f);
        dsp::fft(data, false);

        const std::size_t half = n / 2;
        const double fundamentalEnergy = std::norm(data[fundamentalBin]);
        double totalEnergy = 0.0;
        for (std::size_t k = 1; k < half; ++k)
            totalEnergy += std::norm(data[k]);
        const double nonFundamental = totalEnergy - fundamentalEnergy;
        return nonFundamental / std::max(fundamentalEnergy, 1.0e-9);
    }

    // Counts rising zero-crossings to estimate the fundamental frequency, same
    // technique tests/dsp/ClassicOscillatorTests.cpp already uses.
    double measureFrequencyHz(float freqHz, const PhaseShapeParams& params, double sampleRate, double seconds)
    {
        PhaseShapeOscillator osc;
        osc.prepare(sampleRate);
        osc.reset();
        osc.setFrequency(freqHz);

        const auto numSamples = static_cast<std::size_t>(seconds * sampleRate);
        float prev = osc.renderSample(params);
        int crossings = 0;
        for (std::size_t i = 1; i < numSamples; ++i)
        {
            const float cur = osc.renderSample(params);
            if (prev < 0.0f && cur >= 0.0f)
                ++crossings;
            prev = cur;
        }
        return static_cast<double>(crossings) / seconds;
    }
} // namespace

TEST_CASE("PhaseShapeOscillator tunes correctly at zero warp/fold", "[phaseshape][tuning]")
{
    PhaseShapeParams params; // all defaults -- bend=fold=asymmetry=shape=0, transparent.
    const double measured = measureFrequencyHz(220.0f, params, 48000.0, 1.0);
    REQUIRE(measured == Catch::Approx(220.0).margin(1.0));
}

TEST_CASE("PhaseShapeOscillator output stays bounded and finite across extreme parameters", "[phaseshape][stability]")
{
    const double sampleRate = 48000.0;
    for (float bend : {-1.0f, 0.0f, 1.0f})
        for (float fold : {0.0f, 1.0f})
            for (float asym : {-1.0f, 1.0f})
                for (float shape : {0.0f, 1.0f})
                    for (float freqHz : {20.0f, 440.0f, 8000.0f})
                    {
                        PhaseShapeParams params;
                        params.phaseBend = bend;
                        params.phaseFold = fold;
                        params.phaseAsymmetry = asym;
                        params.phaseShape = shape;

                        const auto samples = renderPhaseShape(freqHz, params, sampleRate, 0, 4096);
                        for (float s : samples)
                        {
                            REQUIRE(std::isfinite(s));
                            REQUIRE(std::abs(s) <= 1.0001f);
                        }
                    }
}

TEST_CASE("PhaseShapeOscillator: wavefold measurably adds harmonic content vs. no fold",
          "[phaseshape][spectral][measured]")
{
    constexpr std::size_t n = 8192;
    constexpr std::size_t warmup = 512; // lets the fold smoother settle before the measurement window.
    // 40 * 48000 / 8192 == 234.375 Hz -- an exact FFT bin, no spectral leakage.
    constexpr float freqHz = 40.0f * 48000.0f / static_cast<float>(n);
    constexpr std::size_t fundamentalBin = 40;

    PhaseShapeParams noFold; // bend=asymmetry=shape=0 -- pure sine carrier.
    PhaseShapeParams withFold;
    withFold.phaseFold = 1.0f;

    const auto plain = renderPhaseShape(freqHz, noFold, 48000.0, warmup, n);
    const auto folded = renderPhaseShape(freqHz, withFold, 48000.0, warmup, n);

    const double plainRatio = nonFundamentalEnergyRatio(plain, fundamentalBin);
    const double foldedRatio = nonFundamentalEnergyRatio(folded, fundamentalBin);

    REQUIRE(plainRatio < 0.05); // a pure sine carrier has essentially no other energy.
    REQUIRE(foldedRatio > plainRatio * 10.0);
}

TEST_CASE("PhaseShapeOscillator: phase bend measurably adds harmonic content vs. zero bend",
          "[phaseshape][spectral][measured]")
{
    constexpr std::size_t n = 8192;
    constexpr std::size_t warmup = 512;
    constexpr float freqHz = 40.0f * 48000.0f / static_cast<float>(n);
    constexpr std::size_t fundamentalBin = 40;

    PhaseShapeParams noBend;
    PhaseShapeParams withBend;
    withBend.phaseBend = 1.0f;

    const auto plain = renderPhaseShape(freqHz, noBend, 48000.0, warmup, n);
    const auto bent = renderPhaseShape(freqHz, withBend, 48000.0, warmup, n);

    const double plainRatio = nonFundamentalEnergyRatio(plain, fundamentalBin);
    const double bentRatio = nonFundamentalEnergyRatio(bent, fundamentalBin);

    REQUIRE(bentRatio > plainRatio * 5.0);
}

TEST_CASE("PhaseShapeOscillator: reset() reproduces the exact same output as a fresh instance", "[phaseshape][determinism]")
{
    PhaseShapeParams params;
    params.phaseBend = 0.6f;
    params.phaseFold = 0.4f;
    params.phaseAsymmetry = -0.3f;
    params.phaseShape = 0.7f;

    PhaseShapeOscillator a;
    a.prepare(48000.0);
    a.reset();
    a.setFrequency(330.0f);

    PhaseShapeOscillator b;
    b.prepare(48000.0);
    b.setFrequency(330.0f);
    // Run b through some state, then reset it -- it should forget everything and
    // match a fresh instance exactly from here on.
    for (int i = 0; i < 500; ++i)
        static_cast<void>(b.renderSample(params));
    b.reset();

    for (int i = 0; i < 2048; ++i)
        REQUIRE(a.renderSample(params) == b.renderSample(params));
}

TEST_CASE("OperatorState renders PhaseShape through the full operator path", "[phaseshape][operator]")
{
    op::OperatorState state;
    state.prepare(48000.0);
    state.reset();

    op::OperatorParams params;
    params.engine = algorithm::EngineType::PhaseShape;
    params.frequencyRatio = 1.0f;
    params.level = 1.0f;
    params.phaseBend = 0.5f;
    params.phaseFold = 0.3f;
    params.phaseAsymmetry = 0.2f;
    params.phaseShape = 0.5f;

    bool sawNonZero = false;
    for (int i = 0; i < 4096; ++i)
    {
        const float s = state.render(params, nullptr, 220.0f, 0.0f, 0.0f);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 1.0001f);
        if (s != 0.0f)
            sawNonZero = true;
    }
    REQUIRE(sawNonZero);
}
