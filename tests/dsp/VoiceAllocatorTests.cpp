#include <catch2/catch_test_macros.hpp>

#include "pw8/render/RenderTypes.hpp"
#include "pw8/voice/VoiceAllocator.hpp"

using namespace pw8;
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
        const auto allocation = allocator.allocate(pool);
        assigned[static_cast<std::size_t>(i)] = allocation.index;
        REQUIRE(allocation.result == AllocateResult::Free);
        pool[allocation.index].noteOn(60 + i, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 42);
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

    const auto alloc0 = allocator.allocate(pool);
    pool[alloc0.index].noteOn(60, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 1);
    const auto alloc1 = allocator.allocate(pool);
    pool[alloc1.index].noteOn(61, 0, 1.0f, 440.0f, env, allocator.nextAge(), 2, 2);

    // Release voice alloc0 and run its envelope down toward zero, then allocate a third
    // note -- it must steal the released voice, not the still-gated one.
    pool[alloc0.index].noteOff(1.0f);
    pw8::algorithm::CompiledAlgorithm compiled;
    [[maybe_unused]] const auto compileStatus = pw8::algorithm::AlgorithmGraphCompiler::compile(
        pw8::algorithm::AlgorithmGraphDefinition::makeDefaultParallel8(), compiled);
    std::array<const pw8::oscillator::WavetableTable*, pw8::core::kNodesPerLayer> tables{};
    std::array<float, pw8::core::kNumLfosPerLayer> layerLfoValues{};
    pw8::core::FixedVector<pw8::modulation::ModRoute, pw8::core::kMaxModRoutes> modRoutes{};
    pw8::core::FixedVector<pw8::patch::MetaModRoute, 8> metaRoutes{};
    // A few samples keeps alloc0 in release (gate off, amp envelope still dying) without going fully idle.
    for (int i = 0; i < 10; ++i)
    {
        float l = 0.0f, r = 0.0f;
        pool[alloc0.index].renderSample(compiled, tables, 120.0f, layerLfoValues,
                                         pw8::modulation::GenerativeOutputValues{}, modRoutes, metaRoutes,
                                         render::QualityMode::Normal, 0.0f, 0.0f, l, r);
    }

    const auto alloc2 = allocator.allocate(pool);
    REQUIRE(alloc2.index == alloc0.index);
    REQUIRE(alloc2.index != alloc1.index);
    REQUIRE(alloc2.result == AllocateResult::Released);
}

TEST_CASE("VoiceAllocator findGatedVoice supports same-note retrigger", "[voice][allocator][retrigger]")
{
    VoicePool pool;
    for (auto& v : pool)
        v.prepare(48000.0);

    VoiceAllocator allocator;
    allocator.configure(4);
    const auto env = quickEnv();

    const auto alloc0 = allocator.allocate(pool);
    pool[alloc0.index].noteOn(60, 0, 0.8f, 440.0f, env, allocator.nextAge(), 1, 1);
    REQUIRE(pool[alloc0.index].gateOn);

    const auto found = allocator.findGatedVoice(pool, 60, 0);
    REQUIRE(found.has_value());
    REQUIRE(*found == alloc0.index);
}

TEST_CASE("VoiceAllocator release() only affects matching note+channel", "[voice][allocator]")
{
    VoicePool pool;
    for (auto& v : pool)
        v.prepare(48000.0);

    VoiceAllocator allocator;
    allocator.configure(2);
    const auto env = quickEnv();

    const auto alloc0 = allocator.allocate(pool);
    pool[alloc0.index].noteOn(60, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 1);
    const auto alloc1 = allocator.allocate(pool);
    pool[alloc1.index].noteOn(61, 0, 1.0f, 440.0f, env, allocator.nextAge(), 2, 2);

    allocator.release(pool, 60, 0, 0.5f);

    REQUIRE_FALSE(pool[alloc0.index].gateOn);
    REQUIRE(pool[alloc1.index].gateOn);
}

TEST_CASE("VoiceAllocator reports Stolen when all voices are actively gated", "[voice][allocator][stealing]")
{
    VoicePool pool;
    for (auto& v : pool)
        v.prepare(48000.0);

    VoiceAllocator allocator;
    allocator.configure(2);
    const auto env = quickEnv();

    const auto alloc0 = allocator.allocate(pool);
    pool[alloc0.index].noteOn(60, 0, 1.0f, 440.0f, env, allocator.nextAge(), 1, 1);
    const auto alloc1 = allocator.allocate(pool);
    pool[alloc1.index].noteOn(61, 0, 1.0f, 440.0f, env, allocator.nextAge(), 2, 2);

    const auto alloc2 = allocator.allocate(pool);
    REQUIRE(alloc2.result == AllocateResult::Stolen);
    REQUIRE((alloc2.index == alloc0.index || alloc2.index == alloc1.index));
}
