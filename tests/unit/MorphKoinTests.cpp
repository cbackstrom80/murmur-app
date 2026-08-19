#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "pw8/modulation/MorphEasing.hpp"
#include "pw8/modulation/MorphKoinExecutor.hpp"
#include "pw8/patch/Patch.hpp"

using namespace pw8;
using namespace pw8::modulation;
using namespace pw8::patch;

TEST_CASE("MorphEasing linear and smooth endpoints", "[modulation][morph][easing]")
{
    REQUIRE(applyMorphEasing(0.0f, MorphEasing::Linear) == Catch::Approx(0.0f));
    REQUIRE(applyMorphEasing(1.0f, MorphEasing::Linear) == Catch::Approx(1.0f));
    REQUIRE(applyMorphEasing(0.5f, MorphEasing::Linear) == Catch::Approx(0.5f));

    REQUIRE(applyMorphEasing(0.0f, MorphEasing::Smooth) == Catch::Approx(0.0f));
    REQUIRE(applyMorphEasing(1.0f, MorphEasing::Smooth) == Catch::Approx(1.0f));
    REQUIRE(applyMorphEasing(0.5f, MorphEasing::Smooth) == Catch::Approx(0.5f));
}

TEST_CASE("MorphEasing inQuartic compresses mid-range", "[modulation][morph][easing]")
{
    REQUIRE(applyMorphEasing(0.5f, MorphEasing::InQuartic) == Catch::Approx(0.0625f));
    REQUIRE(applyMorphEasing(0.5f, "inQuartic") == Catch::Approx(0.0625f));
}

TEST_CASE("MorphEasing outQuartic accelerates toward end", "[modulation][morph][easing]")
{
    REQUIRE(applyMorphEasing(0.5f, MorphEasing::OutQuartic) == Catch::Approx(0.9375f));
}

TEST_CASE("MorphEasing step holds first keyframe until midpoint", "[modulation][morph][easing]")
{
    REQUIRE(applyMorphEasing(0.49f, MorphEasing::Step) == Catch::Approx(0.0f));
    REQUIRE(applyMorphEasing(0.51f, MorphEasing::Step) == Catch::Approx(1.0f));
}

TEST_CASE("MorphEasing parseMorphEasing accepts aliases", "[modulation][morph][easing]")
{
    REQUIRE(parseMorphEasing("smooth") == MorphEasing::Smooth);
    REQUIRE(parseMorphEasing("accelerating") == MorphEasing::InQuartic);
    REQUIRE(parseMorphEasing("decelerating") == MorphEasing::OutQuartic);
    REQUIRE(parseMorphEasing("inOutSine") == MorphEasing::InOutSine);
    REQUIRE(parseMorphEasing("unknown") == MorphEasing::Linear);
}

TEST_CASE("MorphKoinExecutor per-path easing overrides global curve", "[modulation][morph]")
{
    Patch p = Patch::makeInit();
    p.morphKoin.curve = "linear";

    MorphKoinKeyframe kf0;
    kf0.name = "A";
    kf0.position = 0.0f;
    kf0.paramOverrides["filterCutoffHz"] = MorphParamOverride{1000.0f, "inQuartic", {}};

    MorphKoinKeyframe kf1;
    kf1.name = "B";
    kf1.position = 1.0f;
    kf1.paramOverrides["filterCutoffHz"] = MorphParamOverride{2000.0f, {}, {}};

    p.morphKoin.keyframes = {kf0, kf1};

    applyMorphKoin(p, 0.5f);
    const float expected = 1000.0f + (2000.0f - 1000.0f) * 0.0625f;
    REQUIRE(p.layerA.filter1.cutoffHz == Catch::Approx(expected));
}

TEST_CASE("MorphKoinExecutor detectMorphKeyframeCrossing finds forward cross", "[modulation][morph]")
{
    Patch p = Patch::makeInit();
    MorphKoinKeyframe kf0;
    kf0.position = 0.0f;
    MorphKoinKeyframe kf1;
    kf1.position = 0.5f;
    MorphKoinKeyframe kf2;
    kf2.position = 1.0f;
    p.morphKoin.keyframes = {kf0, kf1, kf2};

    REQUIRE(detectMorphKeyframeCrossing(p.morphKoin, 0.2f, 0.6f) == 1);
    REQUIRE(detectMorphKeyframeCrossing(p.morphKoin, 0.6f, 0.4f) == 1);
}

TEST_CASE("MorphKoinExecutor lerps macro values between keyframes", "[modulation][morph]")
{
    Patch p = Patch::makeInit();
    p.morphKoin.label = "EVOLVE";
    p.morphKoin.curve = "linear";

    MorphKoinKeyframe kf0;
    kf0.name = "TIGHT";
    kf0.position = 0.0f;
    kf0.macroValues[0] = 0.1f;
    kf0.hasMacroValues = true;

    MorphKoinKeyframe kf1;
    kf1.name = "VOID";
    kf1.position = 1.0f;
    kf1.macroValues[0] = 0.9f;
    kf1.hasMacroValues = true;

    p.morphKoin.keyframes = {kf0, kf1};

    applyMorphKoin(p, 0.5f);
    REQUIRE(p.macros[0].value == Catch::Approx(0.5f));
}

TEST_CASE("MorphKoinExecutor inQuartic curve reduces mid-segment macro blend", "[modulation][morph]")
{
    Patch p = Patch::makeInit();
    p.morphKoin.curve = "inQuartic";

    MorphKoinKeyframe kf0;
    kf0.name = "A";
    kf0.position = 0.0f;
    kf0.macroValues[0] = 0.0f;
    kf0.hasMacroValues = true;

    MorphKoinKeyframe kf1;
    kf1.name = "B";
    kf1.position = 1.0f;
    kf1.macroValues[0] = 1.0f;
    kf1.hasMacroValues = true;

    p.morphKoin.keyframes = {kf0, kf1};

    applyMorphKoin(p, 0.5f);
    REQUIRE(p.macros[0].value == Catch::Approx(0.0625f));
}
