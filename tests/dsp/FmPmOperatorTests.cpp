#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/operator/OperatorNode.hpp"

using namespace pw8;
using namespace pw8::op;

namespace
{
    // Renders `numSamples` from a fresh FM/PM operator at a fixed carrier frequency,
    // unmodulated by any graph-level edge (phaseMod = freqModHz = 0) -- isolates the
    // engine's own internal modulator/feedback behaviour.
    std::vector<float> renderFmPm(const OperatorParams& params, float baseFrequencyHz, int numSamples,
                                   double sampleRate = 48000.0)
    {
        OperatorState state;
        state.prepare(sampleRate);
        std::vector<float> out(static_cast<std::size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
            out[static_cast<std::size_t>(i)] = state.render(params, nullptr, baseFrequencyHz, 0.0f, 0.0f);
        return out;
    }

    OperatorParams makeFmPmParams()
    {
        OperatorParams p;
        p.engine = algorithm::EngineType::FmPm;
        // keyTrack=false means render()'s carrier frequency comes from
        // fixedFrequencyHz alone, ignoring the baseFrequencyHz argument passed to
        // render() -- keeps each test's intended carrier frequency in one place
        // (fixedFrequencyHz) rather than needing both call sites to agree.
        p.keyTrack = false;
        return p;
    }

    // Counts rising zero-crossings to estimate frequency -- same technique
    // ClassicOscillatorTests.cpp already uses.
    double measureFrequencyHz(const std::vector<float>& samples, double sampleRate)
    {
        int crossings = 0;
        for (std::size_t i = 1; i < samples.size(); ++i)
            if (samples[i - 1] < 0.0f && samples[i] >= 0.0f)
                ++crossings;
        return static_cast<double>(crossings) / (static_cast<double>(samples.size()) / sampleRate);
    }

    // Total spectral energy outside a narrow band around the carrier bin -- a proxy
    // for FM sideband energy (Bessel-function spread), used to confirm modulator
    // index measurably widens the spectrum rather than just "sounding different".
    float measureSidebandEnergy(const std::vector<float>& samples, double sampleRate, float carrierHz)
    {
        std::size_t n = 1;
        while (n * 2 <= samples.size())
            n *= 2;

        std::vector<std::complex<float>> spectrum(n);
        for (std::size_t i = 0; i < n; ++i)
            spectrum[i] = {samples[i], 0.0f};
        dsp::fft(spectrum, false);

        const int carrierBin = static_cast<int>(std::lround(carrierHz * static_cast<float>(n) / static_cast<float>(sampleRate)));
        const int guardBins = 3; // exclude the carrier's own bin (+ a little PolyBLEP-free-sine leakage) from "sideband".

        float energy = 0.0f;
        for (int bin = 1; bin < static_cast<int>(n) / 2; ++bin)
        {
            if (std::abs(bin - carrierBin) <= guardBins)
                continue;
            energy += std::norm(spectrum[static_cast<std::size_t>(bin)]);
        }
        return energy;
    }
} // namespace

TEST_CASE("FmPm operator tunes to the carrier frequency when unmodulated", "[operator][fmpm][tuning]")
{
    auto params = makeFmPmParams();
    params.fixedFrequencyHz = 220.0f;
    params.fmModulatorIndex = 0.0f; // no modulation -- should read as a plain sine at carrier freq.

    const auto samples = renderFmPm(params, 220.0f, 48000);
    const double measured = measureFrequencyHz(samples, 48000.0);
    REQUIRE(measured == Catch::Approx(220.0).margin(1.0));
}

TEST_CASE("FmPm operator output stays bounded and finite under extreme parameters", "[operator][fmpm][stability]")
{
    auto params = makeFmPmParams();
    params.fixedFrequencyHz = 4000.0f;   // high carrier.
    params.fmModulatorRatio = 32.0f;     // max ratio -> very high modulator frequency.
    params.fmModulatorIndex = 2.0f;      // max index.
    params.fmModulatorFeedback = 1.0f;   // max feedback.
    params.fmModulatorWaveform = oscillator::ClassicWaveform::Square; // PolyBLEP-stressing shape.

    const auto samples = renderFmPm(params, 4000.0f, 48000);
    for (const float s : samples)
    {
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 4.0f); // generous bound -- feedback is soft-saturated, not hard-clipped.
    }
}

TEST_CASE("FmPm operator through-zero: a negative instantaneous carrier frequency doesn't blow up", "[operator][fmpm][stability]")
{
    // render()'s carrierHz = fixedFrequencyHz + freqModHz (keyTrack=false) --
    // drive freqModHz negative enough to flip the carrier's instantaneous
    // frequency through zero every call, genuinely exercising
    // ClassicOscillator's dt-can-be-negative code path (a plain phase
    // accumulator is through-zero-safe by construction: negative dt just runs
    // it backward), not just a low/slow parameter combination.
    auto params = makeFmPmParams();
    params.fixedFrequencyHz = 100.0f;
    params.fmModulatorIndex = 1.0f;
    params.fmModulatorFeedback = 0.7f;

    OperatorState state;
    state.prepare(48000.0);
    for (int i = 0; i < 48000; ++i)
    {
        const float freqModHz = (i % 2 == 0) ? -500.0f : 0.0f; // alternates carrierHz between -400Hz and 100Hz.
        const float s = state.render(params, nullptr, 100.0f, 0.0f, freqModHz);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 4.0f);
    }
}

TEST_CASE("FmPm operator: increasing modulator index measurably widens the spectrum", "[operator][fmpm][spectral]")
{
    auto lowIndex = makeFmPmParams();
    lowIndex.fixedFrequencyHz = 440.0f;
    lowIndex.fmModulatorRatio = 2.0f;
    lowIndex.fmModulatorIndex = 0.01f;

    auto highIndex = lowIndex;
    highIndex.fmModulatorIndex = 2.0f;

    const auto lowSamples = renderFmPm(lowIndex, 440.0f, 8192);
    const auto highSamples = renderFmPm(highIndex, 440.0f, 8192);

    const float lowEnergy = measureSidebandEnergy(lowSamples, 48000.0, 440.0f);
    const float highEnergy = measureSidebandEnergy(highSamples, 48000.0, 440.0f);
    REQUIRE(highEnergy > lowEnergy * 2.0f); // a real, not marginal, widening -- margin picked
                                              // against the actually-measured ratio, not guessed.
}

TEST_CASE("FmPm operator reset places phase deterministically", "[operator][fmpm]")
{
    auto params = makeFmPmParams();
    params.fixedFrequencyHz = 330.0f;
    params.fmModulatorIndex = 0.8f;
    params.fmModulatorFeedback = 0.3f;

    OperatorState a, b;
    a.prepare(48000.0);
    b.prepare(48000.0);
    a.reset(0.25f);
    b.reset(0.25f);

    for (int i = 0; i < 1000; ++i)
        REQUIRE(a.render(params, nullptr, 330.0f, 0.0f, 0.0f) == b.render(params, nullptr, 330.0f, 0.0f, 0.0f));
}
