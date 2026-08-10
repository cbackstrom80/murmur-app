#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/patch/PatchSerializer.hpp"

using namespace pw8::patch;

TEST_CASE("Patch roundtrips through JSON", "[patch][serialization]")
{
    Patch p = Patch::makeInit();
    p.metadata.id = "test-id-123";
    p.metadata.name = "Test Patch";
    p.metadata.category = "bass";
    p.metadata.tags = {"warm", "dark"};
    p.seed = 999;
    p.layerA.operators[0].engine = pw8::algorithm::EngineType::Classic;
    p.layerA.operators[0].classicWaveform = pw8::oscillator::ClassicWaveform::Saw;
    p.layerA.ampEnvelope.attackSeconds = 0.05f;
    p.layerA.ampEnvelope.sustainLevel = 0.6f;
    p.voiceSettings.polyphony = 8;

    const auto json = savePatchToJson(p);
    REQUIRE_FALSE(json.empty());

    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.metadata.id == "test-id-123");
    REQUIRE(result.patch.metadata.name == "Test Patch");
    REQUIRE(result.patch.metadata.category == "bass");
    REQUIRE(result.patch.metadata.tags.size() == 2);
    REQUIRE(result.patch.seed == 999);
    REQUIRE(result.patch.layerA.operators[0].classicWaveform == pw8::oscillator::ClassicWaveform::Saw);
    REQUIRE(result.patch.layerA.ampEnvelope.attackSeconds == Catch::Approx(0.05f));
    REQUIRE(result.patch.layerA.ampEnvelope.sustainLevel == Catch::Approx(0.6f));
    REQUIRE(result.patch.voiceSettings.polyphony == 8);
    REQUIRE(result.patch.schemaVersion == pw8::core::kPatchSchemaVersion);
}

TEST_CASE("Patch algorithm graph roundtrips through JSON", "[patch][serialization][algorithm]")
{
    Patch p = Patch::makeInit();
    p.layerA.algorithm.edges.push_back(
        pw8::algorithm::AlgorithmEdge{pw8::core::NodeId(1), pw8::core::NodeId(0), pw8::algorithm::EdgeType::PhaseMod, 0.75f});

    const auto json = savePatchToJson(p);
    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.layerA.algorithm.nodes.size() == pw8::core::kNodesPerLayer);

    bool foundEdge = false;
    for (const auto& e : result.patch.layerA.algorithm.edges)
    {
        if (e.source.get() == 1 && e.destination.get() == 0 && e.type == pw8::algorithm::EdgeType::PhaseMod)
        {
            foundEdge = true;
            REQUIRE(e.amount == Catch::Approx(0.75f));
        }
    }
    REQUIRE(foundEdge);
}

TEST_CASE("loadPatchFromJson rejects malformed input", "[patch][serialization][robustness]")
{
    const auto result = loadPatchFromJson("{not valid json");
    REQUIRE_FALSE(result.ok);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("loadPatchFromJson rejects a non-object root", "[patch][serialization][robustness]")
{
    const auto result = loadPatchFromJson("[1, 2, 3]");
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("loadPatchFromJson fills in sane defaults for a minimal document", "[patch][serialization]")
{
    const auto result = loadPatchFromJson(R"({"schemaVersion": 1})");
    REQUIRE(result.ok);
    REQUIRE(result.patch.layerA.operators.size() == pw8::core::kNodesPerLayer);
    REQUIRE(result.patch.voiceSettings.polyphony >= 1);
}
