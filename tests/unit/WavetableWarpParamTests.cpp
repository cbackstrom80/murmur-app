#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/patch/PatchSerializer.hpp"

using namespace pw8::patch;

TEST_CASE("Patch roundtrip preserves all wavetable warp scalars per operator", "[patch][serialization][warp]")
{
    Patch patch = Patch::makeInit();

    for (std::size_t i = 0; i < pw8::core::kNodesPerLayer; ++i)
    {
        auto& op = patch.layerA.operators[i];
        op.engine = pw8::algorithm::EngineType::Wavetable;
        op.wtBend = static_cast<float>(i) * 0.1f - 0.35f;
        op.wtAsymmetry = static_cast<float>(i) * 0.05f;
        op.wtSyncRatio = 1.0f + static_cast<float>(i);
        op.wtSyncAmount = static_cast<float>(i) * 0.08f;
        op.wtFormantShift = static_cast<float>(i) * 0.07f - 0.2f;
        op.wtMorphMode = static_cast<float>(i % 3);
    }

    const auto json = savePatchToJson(patch);
    const auto loaded = loadPatchFromJson(json);
    REQUIRE(loaded.ok);

    for (std::size_t i = 0; i < pw8::core::kNodesPerLayer; ++i)
    {
        const auto& op = loaded.patch.layerA.operators[i];
        const auto& orig = patch.layerA.operators[i];
        REQUIRE(op.wtBend == Catch::Approx(orig.wtBend));
        REQUIRE(op.wtAsymmetry == Catch::Approx(orig.wtAsymmetry));
        REQUIRE(op.wtSyncRatio == Catch::Approx(orig.wtSyncRatio));
        REQUIRE(op.wtSyncAmount == Catch::Approx(orig.wtSyncAmount));
        REQUIRE(op.wtFormantShift == Catch::Approx(orig.wtFormantShift));
        REQUIRE(op.wtMorphMode == Catch::Approx(orig.wtMorphMode));
    }
}

TEST_CASE("Patch JSON includes wavetable warp keys for DESIGN panel round-trip", "[patch][serialization][warp]")
{
    Patch patch = Patch::makeInit();
    patch.layerA.operators[0].engine = pw8::algorithm::EngineType::Wavetable;
    patch.layerA.operators[0].wtBend = 0.25f;
    patch.layerA.operators[0].wtSyncRatio = 2.5f;

    const auto json = savePatchToJson(patch);
    REQUIRE(json.find("\"wtBend\"") != std::string::npos);
    REQUIRE(json.find("\"wtSyncRatio\"") != std::string::npos);
    REQUIRE(json.find("\"wtFormantShift\"") != std::string::npos);
    REQUIRE(json.find("\"wtMorphMode\"") != std::string::npos);
}
