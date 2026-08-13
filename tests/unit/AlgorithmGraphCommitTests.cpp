#include <catch2/catch_test_macros.hpp>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/patch/Patch.hpp"

// Mirrors PatchworkEightProcessor::commitAlgorithmGraph compile gate — valid edits swap,
// invalid edits leave the patch unchanged.

using namespace pw8;
using namespace pw8::algorithm;

namespace
{
    bool commitAlgorithmGraph(patch::Patch& patch, const AlgorithmGraphDefinition& def) noexcept
    {
        CompiledAlgorithm compiled;
        if (AlgorithmGraphCompiler::compile(def, compiled) != CompileStatus::Ok)
            return false;

        patch.layerA.algorithm = def;
        return true;
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
