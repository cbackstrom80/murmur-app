#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/modulation/MorphKoinExecutor.hpp"
#include "pw8/patch/Patch.hpp"

using namespace pw8;
using namespace pw8::modulation;
using namespace pw8::patch;

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

TEST_CASE("MorphKoinExecutor applies paramOverrides on master quasar slot", "[modulation][morph]")
{
    Patch p = Patch::makeInit();
    p.masterEffects[2].type = effects::EffectType::BinauralSpace;
    p.masterEffects[2].qsr1Distance = 0.35f;

    MorphKoinKeyframe kf0;
    kf0.name = "INTIMATE";
    kf0.position = 0.0f;
    kf0.paramOverrides["masterEffects[2].qsr1Distance"] = 0.1f;

    MorphKoinKeyframe kf1;
    kf1.name = "VOID";
    kf1.position = 1.0f;
    kf1.paramOverrides["masterEffects[2].qsr1Distance"] = 0.85f;

    p.morphKoin.keyframes = {kf0, kf1};

    applyMorphKoin(p, 1.0f);
    REQUIRE(p.masterEffects[2].qsr1Distance == Catch::Approx(0.85f));
}
