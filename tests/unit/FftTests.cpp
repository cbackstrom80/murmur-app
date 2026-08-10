#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/dsp/Fft.hpp"

using namespace pw8::dsp;

TEST_CASE("isPowerOfTwo classifies correctly", "[fft]")
{
    REQUIRE(isPowerOfTwo(1));
    REQUIRE(isPowerOfTwo(2));
    REQUIRE(isPowerOfTwo(1024));
    REQUIRE_FALSE(isPowerOfTwo(0));
    REQUIRE_FALSE(isPowerOfTwo(3));
    REQUIRE_FALSE(isPowerOfTwo(1000));
}

TEST_CASE("fft forward+inverse roundtrips to the original signal", "[fft]")
{
    constexpr std::size_t n = 256;
    std::vector<std::complex<float>> original(n);
    for (std::size_t i = 0; i < n; ++i)
        original[i] = std::complex<float>(std::sin(0.1f * static_cast<float>(i)) + 0.3f * std::cos(0.7f * static_cast<float>(i)), 0.0f);

    auto data = original;
    fft(data, false);
    fft(data, true);

    for (std::size_t i = 0; i < n; ++i)
        REQUIRE(std::abs(data[i] - original[i]) < 1.0e-3f);
}

TEST_CASE("fft of a pure sine at an exact bin frequency peaks only at that bin (and its mirror)", "[fft]")
{
    constexpr std::size_t n = 1024;
    constexpr int k = 17; // bin index -- must divide evenly for a clean single-bin result.

    std::vector<std::complex<float>> data(n);
    for (std::size_t i = 0; i < n; ++i)
        data[i] = std::complex<float>(std::sin(2.0f * 3.14159265f * static_cast<float>(k) * static_cast<float>(i) / static_cast<float>(n)), 0.0f);

    fft(data, false);

    for (std::size_t bin = 0; bin < n; ++bin)
    {
        const float mag = std::abs(data[bin]);
        if (bin == static_cast<std::size_t>(k) || bin == n - static_cast<std::size_t>(k))
            REQUIRE(mag > static_cast<float>(n) / 4.0f); // large peak.
        else
            REQUIRE(mag < static_cast<float>(n) / 100.0f); // near-zero elsewhere.
    }
}

TEST_CASE("fft on a non-power-of-two input is a safe no-op", "[fft][robustness]")
{
    std::vector<std::complex<float>> data(100, std::complex<float>(1.0f, 0.0f));
    const auto copy = data;
    fft(data, false);
    REQUIRE(data == copy); // unchanged, not garbage/crash.
}
