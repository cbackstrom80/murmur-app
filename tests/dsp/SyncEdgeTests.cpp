#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/algorithm/AlgorithmExecutor.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"

using namespace pw8;

namespace
{
    algorithm::CompiledAlgorithm compileSyncGraph()
    {
        algorithm::AlgorithmGraphDefinition def;
        for (std::uint8_t i = 0; i < core::kNodesPerLayer; ++i)
            def.nodes.push_back(algorithm::AlgorithmNode{core::NodeId{i}, algorithm::EngineType::Classic, i == 1});

        algorithm::AlgorithmEdge edge{};
        edge.source = core::NodeId{0};
        edge.destination = core::NodeId{1};
        edge.type = algorithm::EdgeType::Sync;
        edge.amount = 1.0f;
        def.edges.push_back(edge);

        algorithm::CompiledAlgorithm compiled;
        [[maybe_unused]] const auto status = algorithm::AlgorithmGraphCompiler::compile(def, compiled);
        REQUIRE(status == algorithm::CompileStatus::Ok);
        return compiled;
    }
} // namespace

TEST_CASE("AlgorithmExecutor SYNC fires only on source phase wrap, not every sample", "[dsp][sync]")
{
    constexpr double kSampleRate = 48000.0;
    const float sourceHz = 1000.0f; // wraps every ~48 samples at 48 kHz
    const int expectedWrapInterval = static_cast<int>(kSampleRate / sourceHz);

    algorithm::CompiledAlgorithm compiled = compileSyncGraph();

    std::array<op::OperatorParams, core::kNodesPerLayer> params{};
    std::array<op::OperatorState, core::kNodesPerLayer> states{};
    std::array<const oscillator::WavetableTable*, core::kNodesPerLayer> tables{};
    std::array<filter::FilterParams, core::kNodesPerLayer> filterParams{};
    std::array<filter::StateVariableFilter, core::kNodesPerLayer> filters{};

    for (auto& s : states)
        s.prepare(kSampleRate);

    params[0].engine = algorithm::EngineType::Classic;
    params[0].level = 1.0f;
    params[0].classic.waveform = oscillator::ClassicWaveform::Saw;
    params[1].engine = algorithm::EngineType::Classic;
    params[1].level = 1.0f;
    params[1].classic.waveform = oscillator::ClassicWaveform::Saw;

    algorithm::AlgorithmExecutor executor;

    int syncResetCount = 0;
    float prevDestPhase = states[1].classicOsc.getPhase();

    const int numSamples = expectedWrapInterval * 5;
    for (int i = 0; i < numSamples; ++i)
    {
        static_cast<void>(executor.processSample(compiled, params, states, tables, sourceHz, filterParams, filters,
                                                    {}, {}));

        const float destPhase = states[1].classicOsc.getPhase();
        // After a sync reset, destination phase jumps back near zero on the next sample.
        if (destPhase < 0.05f && prevDestPhase > 0.5f)
            ++syncResetCount;
        prevDestPhase = destPhase;
    }

    // Should see roughly one sync reset per source wrap (~5 in 5 cycles), NOT every sample.
    REQUIRE(syncResetCount >= 3);
    REQUIRE(syncResetCount <= 7);
    REQUIRE(syncResetCount < numSamples / 10);
}

TEST_CASE("ClassicOscillator didWrapThisSample tracks phase wrap", "[dsp][sync]")
{
    oscillator::ClassicOscillator osc;
    osc.prepare(48000.0);
    osc.setFrequency(1000.0f);

    oscillator::ClassicOscillatorParams p;
    p.waveform = oscillator::ClassicWaveform::Saw;

    int wrapCount = 0;
    for (int i = 0; i < 480; ++i)
    {
        static_cast<void>(osc.renderSample(p));
        if (osc.didWrapThisSample())
            ++wrapCount;
    }

    // 480 samples at 1 kHz -> ~10 wraps in 10 ms
    REQUIRE(wrapCount >= 8);
    REQUIRE(wrapCount <= 12);
}
