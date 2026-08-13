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
