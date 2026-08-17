#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/dsp/SidechainFollower.hpp"

#include <cmath>
#include <limits>
#include <vector>

using namespace pw8::dsp;

TEST_CASE("SidechainFollower rises on sustained input", "[dsp][sidechain]")
{
    SidechainFollower follower;
    follower.prepare(48000.0);

    std::vector<float> left(512, 0.5f);
    follower.processBlock(left.data(), nullptr, left.size());

    REQUIRE(follower.envelope() > 0.1f);
    REQUIRE(follower.isActive());
}

TEST_CASE("SidechainFollower releases toward zero after input stops", "[dsp][sidechain]")
{
    SidechainFollower follower;
    follower.prepare(48000.0);

    std::vector<float> burst(256, 0.8f);
    follower.processBlock(burst.data(), burst.data(), burst.size());
    const float peak = follower.envelope();
    REQUIRE(peak > 0.2f);

    std::vector<float> silence(48000, 0.0f);
    follower.processBlock(silence.data(), silence.data(), silence.size());
    REQUIRE(follower.envelope() < peak * 0.5f);
}

TEST_CASE("SidechainFollower null left buffer decays envelope", "[dsp][sidechain]")
{
    SidechainFollower follower;
    follower.prepare(48000.0);

    std::vector<float> burst(128, 0.6f);
    follower.processBlock(burst.data(), nullptr, burst.size());
    REQUIRE(follower.isActive());

    const float afterBurst = follower.envelope();
    follower.processBlock(nullptr, nullptr, 48000);
    REQUIRE(follower.envelope() < afterBurst * 0.1f);
}

TEST_CASE("SidechainFollower recovers from NaN host input", "[dsp][sidechain][stability]")
{
    SidechainFollower follower;
    follower.prepare(48000.0);

    std::vector<float> bad(128, std::numeric_limits<float>::quiet_NaN());
    follower.processBlock(bad.data(), bad.data(), bad.size());
    REQUIRE(std::isfinite(follower.envelope()));

    std::vector<float> tone(512, 0.4f);
    follower.processBlock(tone.data(), tone.data(), tone.size());
    REQUIRE(follower.envelope() > 0.05f);
    REQUIRE(std::isfinite(follower.envelope()));
}
