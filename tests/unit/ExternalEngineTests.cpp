#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/algorithm/AlgorithmExecutor.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/filter/StateVariableFilter.hpp"
#include "pw8/operator/OperatorNode.hpp"

using namespace pw8;
using namespace pw8::algorithm;

TEST_CASE("External engine is allowed only on operator 0", "[engine][external]")
{
    REQUIRE(isExternalEngineAllowed(core::NodeId(0), EngineType::External));
    REQUIRE_FALSE(isExternalEngineAllowed(core::NodeId(1), EngineType::External));
    REQUIRE(sanitizeEngineForNode(core::NodeId(3), EngineType::External) == EngineType::Classic);
    REQUIRE(sanitizeEngineForNode(core::NodeId(0), EngineType::External) == EngineType::External);
}

TEST_CASE("External engine input source selects sidechain channel", "[engine][external]")
{
    op::OperatorState state;
    state.prepare(48000.0);

    op::OperatorParams params;
    params.engine = EngineType::External;
    params.level = 0.5f;

    params.externalInputSource = 0.0f;
    REQUIRE(state.render(params, nullptr, 440.0f, 0.0f, 0.0f, 0.8f, -0.4f) == Catch::Approx(0.4f));

    params.externalInputSource = 1.0f;
    REQUIRE(state.render(params, nullptr, 440.0f, 0.0f, 0.0f, 0.8f, -0.4f) == Catch::Approx(-0.2f));

    params.externalInputSource = 2.0f;
    REQUIRE(state.render(params, nullptr, 440.0f, 0.0f, 0.0f, 0.8f, -0.4f) == Catch::Approx(0.1f));
}

TEST_CASE("External op0 direct output excluded from per-voice bus sum", "[engine][external]")
{
    AlgorithmGraphDefinition def = AlgorithmGraphDefinition::makeDefaultParallel8();
    def.nodes[0].engine = EngineType::External;
    def.nodes[0].isOutput = true;

    CompiledAlgorithm compiled;
    REQUIRE(AlgorithmGraphCompiler::compile(def, compiled) == CompileStatus::Ok);
    REQUIRE(compiled.externalOp0DirectOutput);

    std::array<op::OperatorParams, core::kNodesPerLayer> params{};
    std::array<op::OperatorState, core::kNodesPerLayer> states{};
    for (auto& s : states)
        s.prepare(48000.0);
    params[0].engine = EngineType::External;
    params[0].level = 1.0f;
    params[0].externalInputSource = 2.0f;

    std::array<const oscillator::WavetableTable*, core::kNodesPerLayer> tables{};
    std::array<filter::FilterParams, core::kNodesPerLayer> filterParams{};
    std::array<filter::StateVariableFilter, core::kNodesPerLayer> filters{};
    for (auto& f : filters)
        f.prepare(48000.0);

    AlgorithmExecutor executor;
    const float perVoice = executor.processSample(compiled, params, states, tables, 440.0f, filterParams, filters,
                                                  {}, {}, 1.0f, 1.0f);
    REQUIRE(perVoice == Catch::Approx(0.0f));
}

TEST_CASE("External op0 phase-mod edge still receives sidechain per voice", "[engine][external]")
{
    AlgorithmGraphDefinition def = AlgorithmGraphDefinition::makeDefaultParallel8();
    def.nodes[0].engine = EngineType::External;
    def.nodes[0].isOutput = false;
    def.nodes[1].engine = EngineType::Classic;
    def.nodes[1].isOutput = true;
    def.edges.push_back(AlgorithmEdge{core::NodeId(0), core::NodeId(1), EdgeType::PhaseMod, 0.5f});

    CompiledAlgorithm compiled;
    REQUIRE(AlgorithmGraphCompiler::compile(def, compiled) == CompileStatus::Ok);
    REQUIRE_FALSE(compiled.externalOp0DirectOutput);

    std::array<op::OperatorParams, core::kNodesPerLayer> params{};
    std::array<op::OperatorState, core::kNodesPerLayer> states{};
    for (auto& s : states)
        s.prepare(48000.0);
    params[0].engine = EngineType::External;
    params[0].externalInputSource = 2.0f;
    params[1].engine = EngineType::Classic;
    params[1].level = 1.0f;

    std::array<const oscillator::WavetableTable*, core::kNodesPerLayer> tables{};
    std::array<filter::FilterParams, core::kNodesPerLayer> filterParams{};
    std::array<filter::StateVariableFilter, core::kNodesPerLayer> filters{};
    for (auto& f : filters)
        f.prepare(48000.0);

    AlgorithmExecutor executor;
    const float modulated = executor.processSample(compiled, params, states, tables, 440.0f, filterParams, filters,
                                                   {}, {}, 0.5f, 0.5f);
    REQUIRE(std::abs(modulated) > 0.001f);
}

TEST_CASE("AlgorithmExecutor reports per-operator peaks", "[engine][external]")
{
    AlgorithmGraphDefinition def = AlgorithmGraphDefinition::makeDefaultParallel8();
    def.nodes[1].engine = EngineType::Classic;
    def.nodes[1].isOutput = true;

    CompiledAlgorithm compiled;
    REQUIRE(AlgorithmGraphCompiler::compile(def, compiled) == CompileStatus::Ok);

    std::array<op::OperatorParams, core::kNodesPerLayer> params{};
    std::array<op::OperatorState, core::kNodesPerLayer> states{};
    for (auto& s : states)
        s.prepare(48000.0);
    params[0].level = 0.0f;
    params[1].engine = EngineType::Classic;
    params[1].level = 1.0f;

    std::array<const oscillator::WavetableTable*, core::kNodesPerLayer> tables{};
    std::array<filter::FilterParams, core::kNodesPerLayer> filterParams{};
    std::array<filter::StateVariableFilter, core::kNodesPerLayer> filters{};
    for (auto& f : filters)
        f.prepare(48000.0);

    std::array<float, core::kNodesPerLayer> peaks{};
    AlgorithmExecutor executor;
    float out = 0.0f;
    for (int sample = 0; sample < 64; ++sample)
        out = executor.processSample(compiled, params, states, tables, 440.0f, filterParams, filters, {}, {}, 0.0f, 0.0f,
                                     1, &peaks);
    REQUIRE(std::abs(out) > 0.001f);
    REQUIRE(peaks[1] > 0.001f);
    REQUIRE(peaks[0] == Catch::Approx(0.0f));
}
