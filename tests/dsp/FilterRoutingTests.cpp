#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/filter/FilterRouting.hpp"

using namespace pw8::filter;

namespace
{
    constexpr float kTol = 1.0e-5f;

    float mockF1(float x) { return x * 0.5f; }
    float mockF2(float x) { return x * 0.25f; }
} // namespace

TEST_CASE("applyDualFilterRouting serial at 0 matches F1 then F2", "[filter][blades][routing]")
{
    const float out = applyDualFilterRouting(1.0f, 0.0f, true, true, mockF1, mockF2);
    REQUIRE(out == Catch::Approx(mockF2(mockF1(1.0f))).margin(kTol));
}

TEST_CASE("applyDualFilterRouting parallel at 0.5", "[filter][blades][routing]")
{
    const float out = applyDualFilterRouting(1.0f, 0.5f, true, true, mockF1, mockF2);
    const float parallel = 0.5f * (mockF1(1.0f) + mockF2(1.0f));
    REQUIRE(out == Catch::Approx(parallel).margin(kTol));
}

TEST_CASE("applyDualFilterRouting crossfade at 1 blends serial orders", "[filter][blades][routing]")
{
    const float out = applyDualFilterRouting(1.0f, 1.0f, true, true, mockF1, mockF2);
    const float serialF1F2 = mockF2(mockF1(1.0f));
    const float serialF2F1 = mockF1(mockF2(1.0f));
    const float crossfade = 0.5f * (serialF1F2 + serialF2F1);
    REQUIRE(out == Catch::Approx(crossfade).margin(kTol));
}

TEST_CASE("applyDualFilterRouting single-filter bypasses routing morph", "[filter][blades][routing]")
{
    REQUIRE(applyDualFilterRouting(1.0f, 0.75f, true, false, mockF1, mockF2) ==
            Catch::Approx(mockF1(1.0f)).margin(kTol));
    REQUIRE(applyDualFilterRouting(1.0f, 0.25f, false, true, mockF1, mockF2) ==
            Catch::Approx(mockF2(1.0f)).margin(kTol));
}

TEST_CASE("computeFilter2CutoffHz tracks F1 with semitone offset", "[filter][blades][routing]")
{
    CharacterFilterParams f2;
    f2.cutoffHz = 500.0f;
    f2.cutoffOffsetSemitones = 12.0f;

    const float tracked = computeFilter2CutoffHz(true, 1000.0f, f2, 440.0f);
    REQUIRE(tracked == Catch::Approx(2000.0f).margin(kTol));

    const float absolute = computeFilter2CutoffHz(false, 1000.0f, f2, 440.0f);
    REQUIRE(absolute == Catch::Approx(f2.cutoffHz).margin(kTol));
}
