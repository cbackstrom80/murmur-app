#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/operator/OperatorNode.hpp"
#include "pw8/oscillator/AdditiveOscillator.hpp"

using namespace pw8;
using namespace pw8::oscillator;

namespace
{
    std::vector<float> renderAdditive(float freqHz, const AdditiveParams& params, double sampleRate,
                                       std::size_t numSamples)
    {
        AdditiveOscillator osc;
        osc.prepare(sampleRate);
        osc.reset();
        osc.setFrequency(freqHz);

        std::vector<float> out(numSamples);
        for (std::size_t i = 0; i < numSamples; ++i)
            out[i] = osc.renderSample(params);
        return out;
    }

    // Counts rising zero-crossings to estimate the fundamental frequency, same
    // technique tests/dsp/ClassicOscillatorTests.cpp already uses.
    double measureFrequencyHz(float freqHz, const AdditiveParams& params, double sampleRate, double seconds)
    {
        AdditiveOscillator osc;
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

    // Sum of squared FFT magnitude in each of two adjacent harmonic-bin windows,
    // used to compare how much energy sits in the odd vs. even harmonic bins.
    double energyAtHarmonic(const std::vector<std::complex<float>>& spectrum, int harmonicNum, int fundamentalBin)
    {
        const std::size_t bin = static_cast<std::size_t>(harmonicNum * fundamentalBin);
        if (bin >= spectrum.size())
            return 0.0;
        return std::norm(spectrum[bin]);
    }
} // namespace

TEST_CASE("AdditiveOscillator tunes correctly (fundamental frequency)", "[additive][tuning]")
{
    AdditiveParams params;
    params.partialCount = 16;
    const double measured = measureFrequencyHz(220.0f, params, 48000.0, 1.0);
    REQUIRE(measured == Catch::Approx(220.0).margin(1.0));
}

TEST_CASE("AdditiveOscillator output stays bounded and finite across the extreme parameter matrix",
          "[additive][stability]")
{
    const double sampleRate = 48000.0;
    for (int count : {1, 8, 32, 64})
        for (float tilt : {-1.0f, 0.0f, 1.0f})
            for (float oddEven : {0.0f, 1.0f})
                for (float stretch : {-1.0f, 0.0f, 1.0f})
                    for (float freqHz : {20.0f, 440.0f, 8000.0f})
                    {
                        AdditiveParams params;
                        params.partialCount = count;
                        params.tilt = tilt;
                        params.oddEven = oddEven;
                        params.stretch = stretch;

                        const auto samples = renderAdditive(freqHz, params, sampleRate, 2048);
                        for (float s : samples)
                        {
                            REQUIRE(std::isfinite(s));
                            REQUIRE(std::abs(s) <= 4.0f); // generous -- summed-partial normalization headroom.
                        }
                    }
}

TEST_CASE("AdditiveOscillator stays bounded over a multi-second hold (renormalization doesn't drift)",
          "[additive][stability][long]")
{
    AdditiveOscillator osc;
    osc.prepare(48000.0);
    osc.reset();
    osc.setFrequency(220.0f);

    AdditiveParams params;
    params.partialCount = 64; // max partials -- worst case for accumulated coupled-form error.
    params.tilt = 0.0f;
    params.oddEven = 1.0f;
    params.stretch = 0.3f; // non-zero stretch keeps rotation coefficients away from a trivial fixed point.

    // 10 seconds at 48kHz -- several hundred renormalization cycles (every 512
    // samples), enough to catch any accumulation the periodic renormalization
    // doesn't actually correct.
    constexpr int numSamples = 10 * 48000;
    for (int i = 0; i < numSamples; ++i)
    {
        const float s = osc.renderSample(params);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 4.0f);
    }
}

TEST_CASE("AdditiveOscillator: oddEven=0 measurably suppresses even harmonics vs. oddEven=1",
          "[additive][spectral][measured]")
{
    constexpr std::size_t n = 8192;
    // 20 * 48000 / 8192 == 117.1875 Hz -- an exact FFT bin for the fundamental,
    // so every harmonic also lands on an exact bin (harmonic k -> bin 20*k).
    constexpr float freqHz = 20.0f * 48000.0f / static_cast<float>(n);
    constexpr int fundamentalBin = 20;

    AdditiveParams oddOnly;
    oddOnly.partialCount = 8;
    oddOnly.oddEven = 0.0f;

    AdditiveParams full;
    full.partialCount = 8;
    full.oddEven = 1.0f;

    const auto oddSamples = renderAdditive(freqHz, oddOnly, 48000.0, n);
    const auto fullSamples = renderAdditive(freqHz, full, 48000.0, n);

    auto toSpectrum = [](const std::vector<float>& samples) {
        std::vector<std::complex<float>> data(samples.size());
        for (std::size_t i = 0; i < samples.size(); ++i)
            data[i] = std::complex<float>(samples[i], 0.0f);
        dsp::fft(data, false);
        return data;
    };

    const auto oddSpectrum = toSpectrum(oddSamples);
    const auto fullSpectrum = toSpectrum(fullSamples);

    // Harmonic 2 (the 2nd partial) is even -- should be ~absent at oddEven=0 and
    // clearly present at oddEven=1.
    const double oddEnergyAtH2 = energyAtHarmonic(oddSpectrum, 2, fundamentalBin);
    const double fullEnergyAtH2 = energyAtHarmonic(fullSpectrum, 2, fundamentalBin);
    REQUIRE(fullEnergyAtH2 > oddEnergyAtH2 * 50.0);

    // Harmonic 1 (the fundamental, always odd) should be present in both.
    const double oddEnergyAtH1 = energyAtHarmonic(oddSpectrum, 1, fundamentalBin);
    const double fullEnergyAtH1 = energyAtHarmonic(fullSpectrum, 1, fundamentalBin);
    REQUIRE(oddEnergyAtH1 > 0.0);
    REQUIRE(fullEnergyAtH1 > 0.0);
}

TEST_CASE("AdditiveOscillator: positive tilt measurably attenuates upper harmonics relative to the fundamental",
          "[additive][spectral][measured]")
{
    constexpr std::size_t n = 8192;
    constexpr float freqHz = 20.0f * 48000.0f / static_cast<float>(n);
    constexpr int fundamentalBin = 20;

    AdditiveParams flatTilt;
    flatTilt.partialCount = 8;
    flatTilt.tilt = -1.0f; // equal-amplitude harmonics.

    AdditiveParams steepTilt;
    steepTilt.partialCount = 8;
    steepTilt.tilt = 1.0f; // steep 1/n^2 falloff.

    auto toSpectrum = [](const std::vector<float>& samples) {
        std::vector<std::complex<float>> data(samples.size());
        for (std::size_t i = 0; i < samples.size(); ++i)
            data[i] = std::complex<float>(samples[i], 0.0f);
        dsp::fft(data, false);
        return data;
    };

    const auto flatSpectrum = toSpectrum(renderAdditive(freqHz, flatTilt, 48000.0, n));
    const auto steepSpectrum = toSpectrum(renderAdditive(freqHz, steepTilt, 48000.0, n));

    // Ratio of harmonic 5's energy to the fundamental's -- should be much smaller
    // under the steep tilt than under the flat tilt.
    const double flatRatio = energyAtHarmonic(flatSpectrum, 5, fundamentalBin) /
                              std::max(energyAtHarmonic(flatSpectrum, 1, fundamentalBin), 1.0e-9);
    const double steepRatio = energyAtHarmonic(steepSpectrum, 5, fundamentalBin) /
                               std::max(energyAtHarmonic(steepSpectrum, 1, fundamentalBin), 1.0e-9);
    REQUIRE(steepRatio < flatRatio * 0.5);
}

TEST_CASE("AdditiveOscillator: positive stretch measurably shifts an upper partial's actual peak frequency",
          "[additive][spectral][measured]")
{
    // Zero-crossing counting on the full multi-partial sum isn't a reliable
    // frequency probe once several inharmonic partials are ringing together
    // (their beating shifts the composite waveform's crossing rate in ways that
    // don't track any single partial) -- so this test finds harmonic 8's actual
    // spectral peak directly instead, which is what stretch is actually supposed
    // to move.
    constexpr std::size_t n = 65536;
    // Bin 40 at this N/sampleRate -- low enough that harmonic 8 (nominally bin
    // 320) has plenty of neighbouring bins to shift into.
    constexpr float freqHz = 40.0f * 48000.0f / static_cast<float>(n);
    constexpr int nominalHarmonic8Bin = 320; // 8 * 40.

    AdditiveParams noStretch;
    noStretch.partialCount = 8;
    noStretch.stretch = 0.0f;

    AdditiveParams withStretch;
    withStretch.partialCount = 8;
    withStretch.stretch = 1.0f;

    auto toSpectrum = [](const std::vector<float>& samples) {
        std::vector<std::complex<float>> data(samples.size());
        for (std::size_t i = 0; i < samples.size(); ++i)
            data[i] = std::complex<float>(samples[i], 0.0f);
        dsp::fft(data, false);
        return data;
    };

    const auto flatSpectrum = toSpectrum(renderAdditive(freqHz, noStretch, 48000.0, n));
    const auto stretchedSpectrum = toSpectrum(renderAdditive(freqHz, withStretch, 48000.0, n));

    auto peakBinInWindow = [](const std::vector<std::complex<float>>& spectrum, int center, int halfWindow) {
        int bestBin = center;
        float bestMag = 0.0f;
        for (int b = center - halfWindow; b <= center + halfWindow; ++b)
        {
            const float mag = std::abs(spectrum[static_cast<std::size_t>(b)]);
            if (mag > bestMag)
            {
                bestMag = mag;
                bestBin = b;
            }
        }
        return bestBin;
    };

    const int flatPeakBin = peakBinInWindow(flatSpectrum, nominalHarmonic8Bin, 20);
    const int stretchedPeakBin = peakBinInWindow(stretchedSpectrum, nominalHarmonic8Bin, 20);

    // No stretch: harmonic 8 peaks exactly at its nominal bin.
    REQUIRE(flatPeakBin == nominalHarmonic8Bin);
    // Positive stretch sharpens upper partials -- harmonic 8's actual peak
    // should land measurably above its nominal bin.
    REQUIRE(stretchedPeakBin > nominalHarmonic8Bin + 3);
}

TEST_CASE("OperatorState renders Additive through the full operator path", "[additive][operator]")
{
    op::OperatorState state;
    state.prepare(48000.0);
    state.reset();

    op::OperatorParams params;
    params.engine = algorithm::EngineType::Additive;
    params.frequencyRatio = 1.0f;
    params.level = 1.0f;
    params.additivePartialCount = 24.0f;
    params.additiveTilt = 0.0f;
    params.additiveOddEven = 0.5f;
    params.additiveStretch = 0.1f;

    bool sawNonZero = false;
    for (int i = 0; i < 4096; ++i)
    {
        const float s = state.render(params, nullptr, 220.0f, 0.0f, 0.0f);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 4.0f);
        if (s != 0.0f)
            sawNonZero = true;
    }
    REQUIRE(sawNonZero);
}
