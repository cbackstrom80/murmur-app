#include <benchmark/benchmark.h>
#include <cmath>
#include <vector>

#include "pw8/oscillator/ClassicOscillator.hpp"
#include "pw8/oscillator/WavetableOscillator.hpp"

using namespace pw8::oscillator;

namespace
{
    constexpr int kBlockSize = 512;

    void BM_ClassicOscillator_Saw(benchmark::State& state)
    {
        const double sampleRate = static_cast<double>(state.range(0));
        ClassicOscillator osc;
        osc.prepare(sampleRate);
        osc.setFrequency(220.0f);
        ClassicOscillatorParams params;
        params.waveform = ClassicWaveform::Saw;

        for (auto _ : state)
        {
            float sum = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
                sum += osc.renderSample(params);
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * kBlockSize);
    }
    BENCHMARK(BM_ClassicOscillator_Saw)->Arg(44100)->Arg(48000)->Arg(96000);

    void BM_ClassicOscillator_MorphSweep(benchmark::State& state)
    {
        const double sampleRate = static_cast<double>(state.range(0));
        ClassicOscillator osc;
        osc.prepare(sampleRate);
        osc.setFrequency(220.0f);
        ClassicOscillatorParams params;
        params.morph = 0.0f;

        for (auto _ : state)
        {
            float sum = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                params.morph = static_cast<float>(i) / static_cast<float>(kBlockSize);
                sum += osc.renderSample(params);
            }
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * kBlockSize);
    }
    BENCHMARK(BM_ClassicOscillator_MorphSweep)->Arg(48000);

    void BM_WavetableOscillator(benchmark::State& state)
    {
        const double sampleRate = static_cast<double>(state.range(0));
        constexpr int kFrames = 4;
        constexpr int kSamplesPerFrame = 2048;
        static std::vector<float> table = [] {
            std::vector<float> t(kFrames * kSamplesPerFrame);
            for (int f = 0; f < kFrames; ++f)
                for (int i = 0; i < kSamplesPerFrame; ++i)
                    t[static_cast<std::size_t>(f * kSamplesPerFrame + i)] =
                        std::sin(2.0f * 3.14159265f * static_cast<float>(i) / kSamplesPerFrame * static_cast<float>(f + 1));
            return t;
        }();
        WavetableView view{table.data(), kFrames, kSamplesPerFrame};

        WavetableOscillator osc;
        osc.prepare(sampleRate);
        osc.setFrequency(220.0f);

        for (auto _ : state)
        {
            float sum = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
                sum += osc.renderSample(view, 0.5f);
            benchmark::DoNotOptimize(sum);
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * kBlockSize);
    }
    BENCHMARK(BM_WavetableOscillator)->Arg(44100)->Arg(48000)->Arg(96000);

} // namespace

// BENCHMARK_MAIN() lives in RenderBenchmarks.cpp -- all four benchmark source files
// link into the single pw8_benchmarks executable.
