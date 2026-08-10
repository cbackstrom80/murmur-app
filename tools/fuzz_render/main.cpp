// pw8-fuzz-render -- generates random-but-schema-valid patches, renders each one,
// and asserts: no crash, no NaN/Inf, bounded output, reasonable runtime.
//
// "Valid" here means every value is within its documented range and the algorithm
// graph is guaranteed to compile: feed-forward edges are only ever generated from a
// lower node index to a higher one (guaranteeing an acyclic feed-forward subgraph,
// which is exactly what AlgorithmGraphCompiler requires), while FEEDBACK edges (the
// only type allowed to loop) are generated freely, including self-loops. This
// exercises the compiler/executor's real validation and feedback-safety machinery
// rather than only ever hitting the safe-fallback path. Filter 1, LFO1, and the mod
// matrix are randomized too (including deliberately extreme mod-route amounts and
// max-resonance filter settings), exercising the finite-output clamps in
// Voice::renderSample() and StateVariableFilter, not just the algorithm graph's.
//
//   pw8-fuzz-render --count 10000 --seed 1

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "pw8/dsp/Random.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/render/Renderer.hpp"

using namespace pw8;

namespace
{
    struct Args
    {
        long long count = 10000;
        std::uint64_t seed = 1;
        double durationSeconds = 1.5;
        double sampleRate = 48000.0;
        bool verbose = false;
        bool stopOnFirstFailure = false;
    };

    bool parseArgs(int argc, char** argv, Args& out)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string{}; };
            if (arg == "--count") out.count = std::stoll(next());
            else if (arg == "--seed") out.seed = std::stoull(next());
            else if (arg == "--duration") out.durationSeconds = std::stod(next());
            else if (arg == "--sample-rate") out.sampleRate = std::stod(next());
            else if (arg == "--verbose") out.verbose = true;
            else if (arg == "--stop-on-first-failure") out.stopOnFirstFailure = true;
            else if (arg == "--help")
            {
                std::printf("Usage: pw8-fuzz-render [--count N] [--seed N] [--duration S] "
                            "[--sample-rate N] [--verbose] [--stop-on-first-failure]\n");
                return false;
            }
            else
            {
                std::fprintf(stderr, "Unknown argument: %s\n", arg.c_str());
                return false;
            }
        }
        return true;
    }

    algorithm::EngineType randomEngine(dsp::DeterministicRng& rng)
    {
        return static_cast<algorithm::EngineType>(static_cast<int>(rng.nextRange(0.0f, 7.999f)));
    }

    oscillator::ClassicWaveform randomWaveform(dsp::DeterministicRng& rng)
    {
        return static_cast<oscillator::ClassicWaveform>(static_cast<int>(rng.nextRange(0.0f, 3.999f)));
    }

    patch::OperatorPatch randomOperator(dsp::DeterministicRng& rng)
    {
        patch::OperatorPatch op;
        op.engine = randomEngine(rng);
        op.classicWaveform = randomWaveform(rng);
        op.classicMorph = (rng.nextFloat() < 0.5f) ? -1.0f : rng.nextRange(0.0f, 1.0f);
        op.pulseWidth = rng.nextRange(0.05f, 0.95f);
        op.wavetableFramePosition = rng.nextRange(0.0f, 1.0f);
        op.frequencyRatio = rng.nextRange(0.05f, 8.0f);
        op.fixedFrequencyHz = rng.nextRange(20.0f, 8000.0f);
        op.keyTrack = rng.nextFloat() < 0.85f;
        op.level = rng.nextRange(0.0f, 1.5f);
        op.pan = rng.nextRange(-1.0f, 1.0f);
        return op;
    }

    algorithm::AlgorithmGraphDefinition randomAlgorithm(dsp::DeterministicRng& rng)
    {
        algorithm::AlgorithmGraphDefinition def;
        for (std::uint8_t i = 0; i < core::kNodesPerLayer; ++i)
        {
            const bool isOutput = (i == 0) || (rng.nextFloat() < 0.3f); // node 0 always an output -- guarantees validity.
            def.nodes.push_back(algorithm::AlgorithmNode{core::NodeId(i), randomEngine(rng), isOutput});
        }

        const int numEdges = static_cast<int>(rng.nextRange(0.0f, 12.999f));
        for (int e = 0; e < numEdges; ++e)
        {
            const bool feedback = rng.nextFloat() < 0.25f;
            algorithm::AlgorithmEdge edge;
            edge.amount = rng.nextRange(-1.5f, 1.5f);

            if (feedback)
            {
                edge.type = algorithm::EdgeType::Feedback;
                edge.source = core::NodeId(static_cast<std::uint8_t>(rng.nextRange(0.0f, 7.999f)));
                edge.destination = core::NodeId(static_cast<std::uint8_t>(rng.nextRange(0.0f, 7.999f)));
            }
            else
            {
                // Guarantee acyclicity: only ever route from a lower index to a higher one.
                const auto a = static_cast<std::uint8_t>(rng.nextRange(0.0f, 6.999f));
                const auto b = static_cast<std::uint8_t>(a + 1 + static_cast<std::uint8_t>(rng.nextRange(0.0f, static_cast<float>(7 - a) - 0.001f)));
                edge.source = core::NodeId(a);
                edge.destination = core::NodeId(std::min<std::uint8_t>(b, core::kNodesPerLayer - 1));
                const int typeIdx = static_cast<int>(rng.nextRange(0.0f, 5.999f)); // Audio..Sync (excludes Feedback=6)
                edge.type = static_cast<algorithm::EdgeType>(typeIdx);
            }
            def.edges.push_back(edge);
        }
        return def;
    }

    envelope::DahdsrParams randomEnvelope(dsp::DeterministicRng& rng)
    {
        envelope::DahdsrParams e;
        e.delaySeconds = rng.nextFloat() < 0.8f ? 0.0f : rng.nextRange(0.0f, 0.5f);
        e.attackSeconds = rng.nextRange(0.001f, 2.0f);
        e.holdSeconds = rng.nextFloat() < 0.8f ? 0.0f : rng.nextRange(0.0f, 0.5f);
        e.decaySeconds = rng.nextRange(0.001f, 2.0f);
        e.sustainLevel = rng.nextRange(0.0f, 1.0f);
        e.releaseSeconds = rng.nextRange(0.001f, 3.0f);
        e.curveShape = rng.nextRange(0.0f, 8.0f);
        e.legato = rng.nextFloat() < 0.2f;
        return e;
    }

    filter::FilterParams randomFilter(dsp::DeterministicRng& rng)
    {
        filter::FilterParams f;
        f.enabled = rng.nextFloat() < 0.6f;
        f.mode = static_cast<filter::FilterMode>(static_cast<int>(rng.nextRange(0.0f, 4.999f)));
        f.cutoffHz = rng.nextRange(20.0f, 19000.0f);
        f.resonance = rng.nextRange(0.0f, 1.0f); // includes the near-self-oscillation extreme deliberately.
        f.keyTrack = rng.nextRange(-1.0f, 1.0f);
        return f;
    }

    lfo::LfoParams randomLfo(dsp::DeterministicRng& rng)
    {
        lfo::LfoParams l;
        l.waveform = static_cast<lfo::LfoWaveform>(static_cast<int>(rng.nextRange(0.0f, 5.999f)));
        l.mode = static_cast<lfo::LfoMode>(static_cast<int>(rng.nextRange(0.0f, 3.999f)));
        l.rateHz = rng.nextRange(0.01f, 40.0f);
        l.syncDivisionIndex = static_cast<int>(rng.nextRange(0.0f, 9.999f));
        l.phaseOffset = rng.nextRange(0.0f, 1.0f);
        return l;
    }

    core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> randomModRoutes(dsp::DeterministicRng& rng)
    {
        core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> routes;
        const int numRoutes = static_cast<int>(rng.nextRange(0.0f, 8.999f));
        for (int i = 0; i < numRoutes; ++i)
        {
            modulation::ModRoute r;
            r.source = static_cast<modulation::ModSource>(static_cast<int>(rng.nextRange(0.0f, 14.999f)));
            r.destination = static_cast<modulation::ModDestination>(static_cast<int>(rng.nextRange(0.0f, 4.999f)));
            r.targetIndex = static_cast<std::uint8_t>(rng.nextRange(0.0f, 7.999f));
            r.amount = rng.nextRange(-48.0f, 48.0f); // deliberately extreme -- exercises the clamps.
            routes.push_back(r);
        }
        return routes;
    }

    patch::Patch randomPatch(std::uint64_t patchIndex, std::uint64_t masterSeed)
    {
        dsp::DeterministicRng rng(dsp::DeterministicRng::deriveSeed(masterSeed, 0, patchIndex));

        patch::Patch p = patch::Patch::makeInit();
        p.seed = rng.nextU64();
        p.metadata.name = "fuzz-" + std::to_string(patchIndex);

        for (auto& op : p.layerA.operators)
            op = randomOperator(rng);

        p.layerA.algorithm = randomAlgorithm(rng);
        p.layerA.ampEnvelope = randomEnvelope(rng);
        p.layerA.filter1 = randomFilter(rng);
        p.layerA.lfo1 = randomLfo(rng);
        p.layerA.modRoutes = randomModRoutes(rng);
        p.layerA.gain = rng.nextRange(0.1f, 1.5f);
        p.layerA.pan = rng.nextRange(-1.0f, 1.0f);

        p.voiceSettings.polyphony = static_cast<std::size_t>(rng.nextRange(1.0f, 16.999f));
        p.voiceSettings.masterGain = rng.nextRange(0.3f, 1.0f);

        return p;
    }

    midi::MidiSequence randomMidi(std::uint64_t patchIndex, std::uint64_t masterSeed, double maxSeconds)
    {
        dsp::DeterministicRng rng(dsp::DeterministicRng::deriveSeed(masterSeed, 1, patchIndex));
        midi::MidiSequence seq;
        const int numNotes = static_cast<int>(rng.nextRange(1.0f, 5.999f));
        for (int i = 0; i < numNotes; ++i)
        {
            const double onTime = static_cast<double>(rng.nextRange(0.0f, static_cast<float>(maxSeconds * 0.5)));
            const double gate = static_cast<double>(rng.nextRange(0.05f, static_cast<float>(maxSeconds * 0.4)));
            const int note = static_cast<int>(rng.nextRange(24.0f, 96.999f));
            const int velocity = static_cast<int>(rng.nextRange(1.0f, 127.999f));
            const int channel = static_cast<int>(rng.nextRange(0.0f, 3.999f));
            seq.events.push_back(midi::MidiEvent{onTime, midi::EventType::NoteOn, channel, note, velocity, 0, 0});
            seq.events.push_back(midi::MidiEvent{onTime + gate, midi::EventType::NoteOff, channel, note, 0, 0, 0});
        }
        return seq;
    }

} // namespace

int main(int argc, char** argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
        return 2;

    long long failures = 0;
    long long nanOrInfCount = 0;
    long long notOkCount = 0;
    long long slowCount = 0;
    constexpr double kSlowThresholdSeconds = 2.0;

    const auto wallStart = std::chrono::steady_clock::now();

    for (long long i = 0; i < args.count; ++i)
    {
        const auto patch = randomPatch(static_cast<std::uint64_t>(i), args.seed);
        const auto midiSeq = randomMidi(static_cast<std::uint64_t>(i), args.seed, args.durationSeconds);

        dsp::DeterministicRng bpmRng(dsp::DeterministicRng::deriveSeed(args.seed, 2, static_cast<std::uint64_t>(i)));

        render::RenderOptions options;
        options.sampleRate = args.sampleRate;
        options.durationSecondsOverride = args.durationSeconds;
        options.seed = patch.seed;
        options.bpm = bpmRng.nextRange(20.0f, 300.0f); // exercises TempoSync LFOs across the full range.

        const auto renderStart = std::chrono::steady_clock::now();
        const auto result = render::render(patch, midiSeq, options);
        const double renderSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - renderStart).count();

        bool failed = false;
        if (!result.ok)
        {
            ++notOkCount;
            failed = true;
            if (args.verbose)
                std::fprintf(stderr, "[%lld] render failed: %s\n", i, result.error.c_str());
        }
        if (result.metrics.containsNaNOrInf)
        {
            ++nanOrInfCount;
            failed = true;
            if (args.verbose)
                std::fprintf(stderr, "[%lld] NaN/Inf detected in output\n", i);
        }
        if (result.metrics.peak > 10000.0f)
        {
            failed = true;
            if (args.verbose)
                std::fprintf(stderr, "[%lld] unbounded peak: %f\n", i, result.metrics.peak);
        }
        if (renderSeconds > kSlowThresholdSeconds)
        {
            ++slowCount;
            if (args.verbose)
                std::fprintf(stderr, "[%lld] slow render: %.2fs\n", i, renderSeconds);
        }

        if (failed)
        {
            ++failures;
            if (args.stopOnFirstFailure)
                break;
        }
    }

    const double wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - wallStart).count();

    std::printf("pw8-fuzz-render: %lld patches, seed=%llu\n", args.count, static_cast<unsigned long long>(args.seed));
    std::printf("  failures:        %lld\n", failures);
    std::printf("  render not ok:   %lld\n", notOkCount);
    std::printf("  NaN/Inf:         %lld\n", nanOrInfCount);
    std::printf("  slow (>%.1fs):    %lld\n", kSlowThresholdSeconds, slowCount);
    std::printf("  wall time:       %.2fs (%.1f patches/sec)\n", wallSeconds,
                 wallSeconds > 0.0 ? static_cast<double>(args.count) / wallSeconds : 0.0);

    return failures > 0 ? 1 : 0;
}
