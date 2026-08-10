#include <array>
#include <benchmark/benchmark.h>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/voice/Voice.hpp"

using namespace pw8;

namespace
{
    constexpr int kBlockSize = 512;

    void BM_Voice_Render(benchmark::State& state)
    {
        algorithm::CompiledAlgorithm compiled;
        [[maybe_unused]] const auto status =
            algorithm::AlgorithmGraphCompiler::compile(algorithm::AlgorithmGraphDefinition::makeDefaultParallel8(), compiled);
        std::array<oscillator::WavetableView, core::kNodesPerLayer> tables{};

        voice::Voice v;
        v.prepare(48000.0);
        v.operatorParams[0].engine = algorithm::EngineType::Classic;
        v.operatorParams[0].classic.waveform = oscillator::ClassicWaveform::Saw;

        envelope::DahdsrParams env;
        env.attackSeconds = 0.01f;
        env.decaySeconds = 0.2f;
        env.sustainLevel = 0.8f;
        env.releaseSeconds = 0.3f;
        v.noteOn(60, 0, 1.0f, 261.63f, env, 1, 1, 42);

        for (auto _ : state)
        {
            float l = 0.0f, r = 0.0f, sumL = 0.0f, sumR = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                v.renderSample(compiled, tables, l, r);
                sumL += l;
                sumR += r;
            }
            benchmark::DoNotOptimize(sumL);
            benchmark::DoNotOptimize(sumR);
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * kBlockSize);
    }
    BENCHMARK(BM_Voice_Render);

} // namespace
