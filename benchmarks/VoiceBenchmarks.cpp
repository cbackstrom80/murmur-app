#include <array>
#include <benchmark/benchmark.h>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/render/RenderTypes.hpp"
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
        std::array<const oscillator::WavetableTable*, core::kNodesPerLayer> tables{};

        voice::Voice v;
        v.prepare(48000.0);
        v.operatorParams[0].engine = algorithm::EngineType::Classic;
        v.operatorParams[0].classic.waveform = oscillator::ClassicWaveform::Saw;

        std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer> envs{};
        envs[0].attackSeconds = 0.01f;
        envs[0].decaySeconds = 0.2f;
        envs[0].sustainLevel = 0.8f;
        envs[0].releaseSeconds = 0.3f;
        v.noteOn(60, 0, 1.0f, 261.63f, envs, 1, 1, 42);

        std::array<float, core::kNumLfosPerLayer> layerLfoValues{};
        core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> modRoutes{};
        core::FixedVector<patch::MetaModRoute, 8> metaRoutes{};
        for (auto _ : state)
        {
            float l = 0.0f, r = 0.0f, sumL = 0.0f, sumR = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                v.renderSample(compiled, tables, 120.0f, layerLfoValues, modulation::GenerativeOutputValues{}, modRoutes,
                               metaRoutes, render::QualityMode::Normal, 0.0f, 0.0f, l, r);
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
