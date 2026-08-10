#include <catch2/catch_test_macros.hpp>

#include "pw8/dsp/Random.hpp"

using pw8::dsp::DeterministicRng;

TEST_CASE("DeterministicRng is reproducible from the same seed", "[dsp][random][determinism]")
{
    DeterministicRng a(12345);
    DeterministicRng b(12345);

    for (int i = 0; i < 100; ++i)
        REQUIRE(a.nextU64() == b.nextU64());
}

TEST_CASE("DeterministicRng differs across seeds", "[dsp][random]")
{
    DeterministicRng a(1);
    DeterministicRng b(2);
    REQUIRE(a.nextU64() != b.nextU64());
}

TEST_CASE("DeterministicRng::nextFloat stays in [0, 1)", "[dsp][random]")
{
    DeterministicRng rng(999);
    for (int i = 0; i < 10000; ++i)
    {
        const float v = rng.nextFloat();
        REQUIRE(v >= 0.0f);
        REQUIRE(v < 1.0f);
    }
}

TEST_CASE("DeterministicRng::deriveSeed is stable and voice/note dependent", "[dsp][random][determinism]")
{
    const auto s1 = DeterministicRng::deriveSeed(42, 0, 1);
    const auto s2 = DeterministicRng::deriveSeed(42, 0, 1);
    const auto s3 = DeterministicRng::deriveSeed(42, 1, 1);

    REQUIRE(s1 == s2);
    REQUIRE(s1 != s3);
}
