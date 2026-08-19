#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/modulation/MorphEasing.hpp"

using namespace pw8::modulation;

namespace
{
    void requireEndpoints(MorphEasing easing)
    {
        REQUIRE(applyMorphEasing(0.0f, easing) == Catch::Approx(0.0f));
        REQUIRE(applyMorphEasing(1.0f, easing) == Catch::Approx(1.0f));
    }

    void requireMonotonicSamples(MorphEasing easing)
    {
        const float samples[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        float prev = -1.0f;
        for (const float t : samples)
        {
            const float y = applyMorphEasing(t, easing);
            REQUIRE(y >= prev - 1.0e-5f);
            prev = y;
        }
    }
} // namespace

TEST_CASE("MorphEasing LUT endpoints for all curves", "[MorphEasing]")
{
    requireEndpoints(MorphEasing::Linear);
    requireEndpoints(MorphEasing::Smooth);
    requireEndpoints(MorphEasing::InQuartic);
    requireEndpoints(MorphEasing::OutQuartic);
    requireEndpoints(MorphEasing::InOutSine);
    requireEndpoints(MorphEasing::Bounce);
}

TEST_CASE("MorphEasing LUT samples at 0.25/0.5/0.75", "[MorphEasing]")
{
    REQUIRE(applyMorphEasing(0.25f, MorphEasing::Linear) == Catch::Approx(0.25f));
    REQUIRE(applyMorphEasing(0.75f, MorphEasing::Linear) == Catch::Approx(0.75f));
    REQUIRE(applyMorphEasing(0.5f, MorphEasing::Smooth) == Catch::Approx(0.5f));
    REQUIRE(applyMorphEasing(0.25f, MorphEasing::InQuartic) == Catch::Approx(0.00390625f));
    REQUIRE(applyMorphEasing(0.75f, MorphEasing::OutQuartic) == Catch::Approx(0.99609375f));
    REQUIRE(applyMorphEasing(0.5f, MorphEasing::InOutSine) == Catch::Approx(0.5f));
}

TEST_CASE("MorphEasing step midpoint threshold", "[MorphEasing]")
{
    REQUIRE(applyMorphEasing(0.49f, MorphEasing::Step) == Catch::Approx(0.0f));
    REQUIRE(applyMorphEasing(0.51f, MorphEasing::Step) == Catch::Approx(1.0f));
}

TEST_CASE("MorphEasing monotonic except step", "[MorphEasing]")
{
    requireMonotonicSamples(MorphEasing::Linear);
    requireMonotonicSamples(MorphEasing::Smooth);
    requireMonotonicSamples(MorphEasing::InQuartic);
    requireMonotonicSamples(MorphEasing::OutQuartic);
    requireMonotonicSamples(MorphEasing::InOutSine);
    requireMonotonicSamples(MorphEasing::Bounce);
}

TEST_CASE("MorphEasing string alias parity", "[MorphEasing]")
{
    REQUIRE(applyMorphEasing(0.5f, "smooth") == Catch::Approx(applyMorphEasing(0.5f, MorphEasing::Smooth)));
    REQUIRE(applyMorphEasing(0.5f, "inQuartic") == Catch::Approx(applyMorphEasing(0.5f, MorphEasing::InQuartic)));
    REQUIRE(parseMorphEasing("decelerating") == MorphEasing::OutQuartic);
}
