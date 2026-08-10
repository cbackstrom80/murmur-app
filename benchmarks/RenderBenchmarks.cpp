#include <benchmark/benchmark.h>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Renderer.hpp"

using namespace pw8;

namespace
{
    midi::MidiSequence makeChordSequence(int numNotes)
    {
        midi::MidiSequence seq;
        for (int i = 0; i < numNotes; ++i)
        {
            seq.events.push_back(midi::MidiEvent{0.0, midi::EventType::NoteOn, 0, 48 + i, 100, 0, 0});
            seq.events.push_back(midi::MidiEvent{1.0, midi::EventType::NoteOff, 0, 48 + i, 0, 0, 0});
        }
        return seq;
    }

    // Full patch render at a given (sampleRate, polyphony) pair, matching the master
    // spec's benchmark matrix: 44.1/48/96 kHz x 1/8/16/32 voices.
    void BM_FullPatch_Render(benchmark::State& state)
    {
        const double sampleRate = static_cast<double>(state.range(0));
        const int polyphony = static_cast<int>(state.range(1));

        patch::Patch p = patch::Patch::makeInit();
        p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
        p.voiceSettings.polyphony = static_cast<std::size_t>(polyphony);

        const auto midiSeq = makeChordSequence(polyphony);

        render::RenderOptions options;
        options.sampleRate = sampleRate;
        options.durationSecondsOverride = 1.0;

        for (auto _ : state)
        {
            const auto result = render::render(p, midiSeq, options);
            float peak = result.metrics.peak;
            benchmark::DoNotOptimize(peak);
        }
        state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) *
                                 static_cast<std::int64_t>(sampleRate));
    }
    BENCHMARK(BM_FullPatch_Render)
        ->Args({44100, 1})
        ->Args({44100, 8})
        ->Args({44100, 16})
        ->Args({44100, 32})
        ->Args({48000, 1})
        ->Args({48000, 8})
        ->Args({48000, 16})
        ->Args({48000, 32})
        ->Args({96000, 1})
        ->Args({96000, 8})
        ->Args({96000, 16})
        ->Args({96000, 32})
        ->Unit(benchmark::kMillisecond);

} // namespace

BENCHMARK_MAIN();
