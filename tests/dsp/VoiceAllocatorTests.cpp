#include <catch2/catch_test_macros.hpp>

#include "pw8/voice/VoiceAllocator.hpp"

using namespace pw8::voice;

namespace
{
    /// 8-envelope array Voice::noteOn() now expects -- only index 0 (the amp
    /// envelope) matters for these allocator-focused tests, the rest stay default.
    std::array<pw8::envelope::DahdsrParams, pw8::core::kNumEnvelopesPerLayer> quickEnv()
    {
        std::array<pw8::envelope::DahdsrParams, pw8::core::kNumEnvelopesPerLayer> envs{};
        envs[0].attackSeconds = 0.001f;
        envs[0].decaySeconds = 0.001f;
        envs[0].sustainLevel = 1.0f;
        envs[0].releaseSeconds = 0.001f;
        return envs;
    }
} // namespace

TEST_CASE("VoiceAllocator assigns free voices before stealing", "[voice][allocator]")
{
    VoicePool pool;
    for (auto& v : pool)
        v.prepare(48000.0);

    VoiceAllocator allocator;
    allocator.configure(4);

    const auto env = quickEnv();
    std::array<std::size_t, 4> assigned{};
    for (int i = 0; i < 4; ++i)
    {
        const auto idx = allocator.allocate(pool);
        assigned[static_cast<std::size_t>(i)] = idx;
        pool[idx].noteOn(60 + i, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 42);
    }

    // All 4 assigned indices should be distinct -- no voice reused while free ones exist.
    for (std::size_t i = 0; i < 4; ++i)
        for (std::size_t j = i + 1; j < 4; ++j)
            REQUIRE(assigned[i] != assigned[j]);
}

TEST_CASE("VoiceAllocator steals a released voice over an actively-gated one", "[voice][allocator][stealing]")
{
    VoicePool pool;
    for (auto& v : pool)
        v.prepare(48000.0);

    VoiceAllocator allocator;
    allocator.configure(2);

    const auto env = quickEnv();

    const auto idx0 = allocator.allocate(pool);
    pool[idx0].noteOn(60, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 1);
    const auto idx1 = allocator.allocate(pool);
    pool[idx1].noteOn(61, 0, 1.0f, 440.0f, env, allocator.nextAge(), 2, 2);

    // Release voice idx0 and run its envelope down toward zero, then allocate a third
    // note -- it must steal the released voice, not the still-gated one.
    pool[idx0].noteOff(1.0f);
    pw8::algorithm::CompiledAlgorithm compiled;
    [[maybe_unused]] const auto compileStatus = pw8::algorithm::AlgorithmGraphCompiler::compile(
        pw8::algorithm::AlgorithmGraphDefinition::makeDefaultParallel8(), compiled);
    std::array<const pw8::oscillator::WavetableTable*, pw8::core::kNodesPerLayer> tables{};
    std::array<float, pw8::core::kNumLfosPerLayer> layerLfoValues{};
    for (int i = 0; i < 10000; ++i)
    {
        float l = 0.0f, r = 0.0f;
        pool[idx0].renderSample(compiled, tables, 120.0f, layerLfoValues, l, r);
    }

    const auto idx2 = allocator.allocate(pool);
    REQUIRE(idx2 == idx0);
    REQUIRE(idx2 != idx1);
}

TEST_CASE("VoiceAllocator release() only affects matching note+channel", "[voice][allocator]")
{
    VoicePool pool;
    for (auto& v : pool)
        v.prepare(48000.0);

    VoiceAllocator allocator;
    allocator.configure(2);
    const auto env = quickEnv();

    const auto idx0 = allocator.allocate(pool);
    pool[idx0].noteOn(60, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 1);
    const auto idx1 = allocator.allocate(pool);
    pool[idx1].noteOn(61, 0, 1.0f, 440.0f, env, allocator.nextAge(), 2, 2);

    allocator.release(pool, 60, 0, 0.5f);

    REQUIRE_FALSE(pool[idx0].gateOn);
    REQUIRE(pool[idx1].gateOn);
}
