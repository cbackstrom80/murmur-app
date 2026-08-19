#include <catch2/catch_test_macros.hpp>

#include "pw8/modulation/GenerativeSources.hpp"

TEST_CASE("GenerativeProcessor seeded streams are deterministic", "[modulation][generative]")
{
    pw8::modulation::GenerativeParams params{};
    params.seed = 42;
    params.dejaVu = true;
    params.clockTRateHz = 10.0f;
    params.clockXRateHz = 20.0f;
    params.streams[0].lagMs = 0.1f;

    pw8::modulation::GenerativeProcessor procA;
    pw8::modulation::GenerativeProcessor procB;
    procA.prepare(48000.0);
    procB.prepare(48000.0);
    procA.reset(params, 99);
    procB.reset(params, 99);

    for (int i = 0; i < 128; ++i)
    {
        const auto a = procA.processSample(params, 99);
        const auto b = procB.processSample(params, 99);
        REQUIRE(a.randomT == b.randomT);
        REQUIRE(a.randomX == b.randomX);
    }
}

TEST_CASE("GenerativeProcessor T clock advances outputs", "[modulation][generative]")
{
    pw8::modulation::GenerativeProcessor proc;
    proc.prepare(48000.0);

    pw8::modulation::GenerativeParams params{};
    params.seed = 7;
    params.dejaVu = false;
    params.clockTRateHz = 48000.0f;
    params.clockXRateHz = 0.01f;

    proc.reset(params, 1);
    const auto a = proc.processSample(params, 1);
    const auto b = proc.processSample(params, 1);

    REQUIRE(a.randomT != b.randomT);
}

TEST_CASE("GenerativeProcessor correlation blends X toward T", "[modulation][generative]")
{
    pw8::modulation::GenerativeProcessor proc;
    proc.prepare(48000.0);

    pw8::modulation::GenerativeParams low{};
    low.seed = 123;
    low.dejaVu = false;
    low.correlation = 0.0f;
    low.clockTRateHz = 48000.0f;
    low.clockXRateHz = 48000.0f;

    pw8::modulation::GenerativeParams high = low;
    high.correlation = 1.0f;

    proc.reset(low, 1);
    (void)proc.processSample(low, 1);
    const auto uncorr = proc.processSample(low, 1);

    proc.reset(high, 1);
    (void)proc.processSample(high, 1);
    const auto corr = proc.processSample(high, 1);

    REQUIRE(std::abs(corr.randomX - corr.randomT) <= std::abs(uncorr.randomX - uncorr.randomT) + 0.001f);
}
