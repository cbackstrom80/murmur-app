#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/patch/Patch.hpp"

// Mirrors MurmurProcessor::commitAlgorithmGraph — valid edits swap,
// invalid edits leave the patch unchanged, and APVTS-backed fields sync before reload.

using namespace pw8;
using namespace pw8::algorithm;

namespace
{
    /// Stand-in for host-facing APVTS values that may diverge from `currentPatch_`
    /// between the last sync and a graph commit (e.g. warp knobs edited in DESIGN).
    struct LiveApvtsSnapshot
    {
        float wtBend = 0.0f;
    };

    void syncPatchFromApvts(patch::Patch& patch, const LiveApvtsSnapshot& apvts) noexcept
    {
        patch.layerA.operators[0].wtBend = apvts.wtBend;
    }

    void syncApvtsFromPatch(const patch::Patch& patch, LiveApvtsSnapshot& apvts) noexcept
    {
        apvts.wtBend = patch.layerA.operators[0].wtBend;
    }

    bool commitAlgorithmGraph(patch::Patch& patch, LiveApvtsSnapshot& apvts, const AlgorithmGraphDefinition& def,
                              bool syncBeforeLoad) noexcept
    {
        CompiledAlgorithm compiled;
        if (AlgorithmGraphCompiler::compile(def, compiled) != CompileStatus::Ok)
            return false;

        if (syncBeforeLoad)
            syncPatchFromApvts(patch, apvts);
        patch.layerA.algorithm = def;
        syncApvtsFromPatch(patch, apvts); // loadPatch -> syncAllParametersFromPatch
        return true;
    }

    bool commitAlgorithmGraph(patch::Patch& patch, const AlgorithmGraphDefinition& def) noexcept
    {
        LiveApvtsSnapshot apvts;
        return commitAlgorithmGraph(patch, apvts, def, true);
    }

    AlgorithmGraphDefinition makeValidBellGraph()
    {
        auto def = AlgorithmGraphDefinition::makeDefaultParallel8();
        for (auto& node : def.nodes)
            node.isOutput = false;
        def.nodes[0].isOutput = true;
        def.edges.push_back(AlgorithmEdge{core::NodeId(2), core::NodeId(1), EdgeType::PhaseMod, 0.8f});
        def.edges.push_back(AlgorithmEdge{core::NodeId(1), core::NodeId(1), EdgeType::Feedback, 0.6f});
        def.edges.push_back(AlgorithmEdge{core::NodeId(1), core::NodeId(0), EdgeType::PhaseMod, 1.0f});
        return def;
    }

    AlgorithmGraphDefinition makeCyclicGraph()
    {
        auto def = AlgorithmGraphDefinition::makeDefaultParallel8();
        def.nodes[0].isOutput = true;
        def.edges.push_back(AlgorithmEdge{core::NodeId(0), core::NodeId(1), EdgeType::Audio, 1.0f});
        def.edges.push_back(AlgorithmEdge{core::NodeId(1), core::NodeId(2), EdgeType::PhaseMod, 1.0f});
        def.edges.push_back(AlgorithmEdge{core::NodeId(2), core::NodeId(0), EdgeType::Audio, 1.0f});
        return def;
    }
} // namespace

TEST_CASE("commitAlgorithmGraph swaps patch on valid compile", "[algorithm][processor][commit]")
{
    patch::Patch patch = patch::Patch::makeInit();
    const auto originalEdgeCount = patch.layerA.algorithm.edges.size();

    const auto bell = makeValidBellGraph();
    REQUIRE(commitAlgorithmGraph(patch, bell));

    REQUIRE(patch.layerA.algorithm.edges.size() == 3);
    REQUIRE(patch.layerA.algorithm.edges.size() != originalEdgeCount);
}

TEST_CASE("commitAlgorithmGraph leaves patch unchanged on compile failure", "[algorithm][processor][commit]")
{
    patch::Patch patch = patch::Patch::makeInit();
    const auto snapshot = patch.layerA.algorithm;

    const auto cyclic = makeCyclicGraph();
    REQUIRE_FALSE(commitAlgorithmGraph(patch, cyclic));

    REQUIRE(patch.layerA.algorithm.edges.size() == snapshot.edges.size());
    for (std::size_t i = 0; i < snapshot.edges.size(); ++i)
    {
        REQUIRE(patch.layerA.algorithm.edges[i].source.get() == snapshot.edges[i].source.get());
        REQUIRE(patch.layerA.algorithm.edges[i].destination.get() == snapshot.edges[i].destination.get());
        REQUIRE(patch.layerA.algorithm.edges[i].type == snapshot.edges[i].type);
    }
}

TEST_CASE("commitAlgorithmGraph preserves isOutput flags", "[algorithm][processor][commit][sync]")
{
    patch::Patch patch = patch::Patch::makeInit();
    auto def = patch.layerA.algorithm;
    for (std::size_t i = 0; i < def.nodes.size(); ++i)
        def.nodes[i].isOutput = (i == 0 || i == 3);

    REQUIRE(commitAlgorithmGraph(patch, def));
    REQUIRE(patch.layerA.algorithm.nodes[0].isOutput);
    REQUIRE_FALSE(patch.layerA.algorithm.nodes[1].isOutput);
    REQUIRE(patch.layerA.algorithm.nodes[3].isOutput);
}

TEST_CASE("commitAlgorithmGraph preserves APVTS warp edits after reload sync", "[algorithm][processor][commit][sync]")
{
    patch::Patch patch = patch::Patch::makeInit();
    patch.layerA.operators[0].wtBend = 0.0f; // stale currentPatch_ value

    LiveApvtsSnapshot apvts;
    apvts.wtBend = 0.75f; // user moved the DESIGN warp knob since last sync

    const auto bell = makeValidBellGraph();
    REQUIRE(commitAlgorithmGraph(patch, apvts, bell, true));

    REQUIRE(apvts.wtBend == Catch::Approx(0.75f));
    REQUIRE(patch.layerA.operators[0].wtBend == Catch::Approx(0.75f));
}

TEST_CASE("commitAlgorithmGraph without APVTS sync reverts warp knob edits", "[algorithm][processor][commit][sync]")
{
    patch::Patch patch = patch::Patch::makeInit();
    patch.layerA.operators[0].wtBend = 0.0f;

    LiveApvtsSnapshot apvts;
    apvts.wtBend = 0.75f;

    const auto bell = makeValidBellGraph();
    REQUIRE(commitAlgorithmGraph(patch, apvts, bell, false));

    REQUIRE(apvts.wtBend == Catch::Approx(0.0f)); // documents the pre-fix regression
}

TEST_CASE("algorithm node engines stay aligned with operators after sync", "[algorithm][processor][sync]")
{
    patch::Patch patch = patch::Patch::makeInit();
    patch.layerA.operators[2].engine = algorithm::EngineType::Wavetable;
    patch.layerA.operators[5].engine = algorithm::EngineType::FmPm;

    for (std::size_t i = 0; i < patch::Patch::makeInit().layerA.algorithm.nodes.size(); ++i)
        patch.layerA.algorithm.nodes[i].engine = patch.layerA.operators[i].engine;

    REQUIRE(patch.layerA.algorithm.nodes[2].engine == algorithm::EngineType::Wavetable);
    REQUIRE(patch.layerA.algorithm.nodes[5].engine == algorithm::EngineType::FmPm);
}
