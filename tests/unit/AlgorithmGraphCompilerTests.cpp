#include <catch2/catch_test_macros.hpp>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"

using namespace pw8::algorithm;
using pw8::core::NodeId;

namespace
{
    AlgorithmGraphDefinition makeNodes()
    {
        AlgorithmGraphDefinition def;
        for (std::uint8_t i = 0; i < pw8::core::kNodesPerLayer; ++i)
            def.nodes.push_back(AlgorithmNode{NodeId(i), EngineType::Classic, /*isOutput=*/false});
        return def;
    }
} // namespace

TEST_CASE("AlgorithmGraphCompiler compiles the default parallel-8 template", "[algorithm][compiler]")
{
    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(AlgorithmGraphDefinition::makeDefaultParallel8(), out);
    REQUIRE(status == CompileStatus::Ok);
    REQUIRE(out.isValid);
    REQUIRE(out.executionOrder.size() == pw8::core::kNodesPerLayer);
    REQUIRE(out.outputNodes.size() == 1);
}

TEST_CASE("AlgorithmGraphCompiler rejects a feed-forward cycle", "[algorithm][compiler][cycle]")
{
    auto def = makeNodes();
    def.nodes[0].isOutput = true;
    def.edges.push_back(AlgorithmEdge{NodeId(0), NodeId(1), EdgeType::Audio, 1.0f});
    def.edges.push_back(AlgorithmEdge{NodeId(1), NodeId(2), EdgeType::PhaseMod, 1.0f});
    def.edges.push_back(AlgorithmEdge{NodeId(2), NodeId(0), EdgeType::Audio, 1.0f}); // closes the cycle.

    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::FeedForwardCycle);
    REQUIRE_FALSE(out.isValid);
}

TEST_CASE("AlgorithmGraphCompiler allows an equivalent loop when marked FEEDBACK", "[algorithm][compiler][feedback]")
{
    auto def = makeNodes();
    def.nodes[0].isOutput = true;
    def.edges.push_back(AlgorithmEdge{NodeId(0), NodeId(1), EdgeType::Audio, 1.0f});
    def.edges.push_back(AlgorithmEdge{NodeId(1), NodeId(2), EdgeType::PhaseMod, 1.0f});
    def.edges.push_back(AlgorithmEdge{NodeId(2), NodeId(0), EdgeType::Feedback, 0.5f}); // now delayed, not a real cycle.

    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::Ok);
    REQUIRE(out.feedbackEdges.size() == 1);
    REQUIRE(out.feedForwardEdges.size() == 2);
}

TEST_CASE("AlgorithmGraphCompiler allows self-feedback", "[algorithm][compiler][feedback]")
{
    auto def = makeNodes();
    def.nodes[0].isOutput = true;
    def.edges.push_back(AlgorithmEdge{NodeId(0), NodeId(0), EdgeType::Feedback, 0.3f});

    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::Ok);
    REQUIRE(out.feedbackEdges.size() == 1);
}

TEST_CASE("AlgorithmGraphCompiler requires at least one output node", "[algorithm][compiler][validation]")
{
    auto def = makeNodes(); // no node marked isOutput.
    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::NoOutputNodes);
}

TEST_CASE("AlgorithmGraphCompiler rejects duplicate node IDs", "[algorithm][compiler][validation]")
{
    AlgorithmGraphDefinition def;
    for (std::uint8_t i = 0; i < pw8::core::kNodesPerLayer; ++i)
        def.nodes.push_back(AlgorithmNode{NodeId(0), EngineType::Classic, i == 0}); // all id 0 -- duplicate.

    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::DuplicateNodeId);
}

TEST_CASE("AlgorithmGraphCompiler rejects an edge referencing an out-of-range node", "[algorithm][compiler][validation]")
{
    auto def = makeNodes();
    def.nodes[0].isOutput = true;
    def.edges.push_back(AlgorithmEdge{NodeId(0), NodeId(200), EdgeType::Audio, 1.0f});

    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::InvalidEdgeReference);
}

TEST_CASE("AlgorithmGraphCompiler eliminates zero-value edges", "[algorithm][compiler]")
{
    auto def = makeNodes();
    def.nodes[0].isOutput = true;
    def.edges.push_back(AlgorithmEdge{NodeId(1), NodeId(0), EdgeType::Audio, 0.0f});

    CompiledAlgorithm out;
    const auto status = AlgorithmGraphCompiler::compile(def, out);
    REQUIRE(status == CompileStatus::Ok);
    REQUIRE(out.feedForwardEdges.empty());
}
