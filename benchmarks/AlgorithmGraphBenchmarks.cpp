#include <array>
#include <benchmark/benchmark.h>

#include "pw8/algorithm/AlgorithmExecutor.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/operator/OperatorNode.hpp"

using namespace pw8;

namespace
{
    constexpr int kBlockSize = 512;

    algorithm::AlgorithmGraphDefinition makeSerial8()
    {
        algorithm::AlgorithmGraphDefinition def;
        for (std::uint8_t i = 0; i < core::kNodesPerLayer; ++i)
            def.nodes.push_back(algorithm::AlgorithmNode{core::NodeId(i), algorithm::EngineType::Classic, i == 0});
        for (std::uint8_t i = core::kNodesPerLayer - 1; i > 0; --i)
            def.edges.push_back(algorithm::AlgorithmEdge{core::NodeId(i), core::NodeId(static_cast<std::uint8_t>(i - 1)),
                                                           algorithm::EdgeType::PhaseMod, 1.0f});
        return def;
    }

    algorithm::AlgorithmGraphDefinition makeFeedbackBell()
    {
        algorithm::AlgorithmGraphDefinition def;
        for (std::uint8_t i = 0; i < core::kNodesPerLayer; ++i)
            def.nodes.push_back(algorithm::AlgorithmNode{core::NodeId(i), algorithm::EngineType::Classic, i == 0});
        def.edges.push_back(algorithm::AlgorithmEdge{core::NodeId(2), core::NodeId(1), algorithm::EdgeType::PhaseMod, 0.8f});
        def.edges.push_back(algorithm::AlgorithmEdge{core::NodeId(1), core::NodeId(1), algorithm::EdgeType::Feedback, 0.6f});
        def.edges.push_back(algorithm::AlgorithmEdge{core::NodeId(1), core::NodeId(0), algorithm::EdgeType::PhaseMod, 1.0f});
        return def;
    }

    void runGraphBenchmark(benchmark::State& state, const algorithm::AlgorithmGraphDefinition& def)
    {
        algorithm::CompiledAlgorithm compiled;
        [[maybe_unused]] const auto status = algorithm::AlgorithmGraphCompiler::compile(def, compiled);

        std::array<op::OperatorParams, core::kNodesPerLayer> params{};
        std::array<op::OperatorState, core::kNodesPerLayer> states{};
        std::array<oscillator::WavetableView, core::kNodesPerLayer> tables{};
        for (auto& s : states)
            s.prepare(48000.0);

        algorithm::AlgorithmExecutor executor;

        for (auto _ : state)
        {
            float sum = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
                sum += executor.processSample(compiled, params, states, tables, 220.0f);
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * kBlockSize);
    }

    void BM_AlgorithmGraph_Parallel8(benchmark::State& state)
    {
        runGraphBenchmark(state, algorithm::AlgorithmGraphDefinition::makeDefaultParallel8());
    }
    BENCHMARK(BM_AlgorithmGraph_Parallel8);

    void BM_AlgorithmGraph_Serial8(benchmark::State& state) { runGraphBenchmark(state, makeSerial8()); }
    BENCHMARK(BM_AlgorithmGraph_Serial8);

    void BM_AlgorithmGraph_FeedbackBell(benchmark::State& state) { runGraphBenchmark(state, makeFeedbackBell()); }
    BENCHMARK(BM_AlgorithmGraph_FeedbackBell);

} // namespace
