#include <memory>
#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <vector>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/BlockMidi.hpp"
#include "pw8/render/Engine.hpp"

using namespace pw8;

namespace
{
    constexpr double kSampleRate = 48000.0;

    [[nodiscard]] std::uint64_t fingerprintAudio(const std::vector<float>& left, const std::vector<float>& right) noexcept
    {
        std::uint64_t hash = 14695981039346656037ULL;
        for (float sample : left)
        {
            const auto bits = std::bit_cast<std::uint32_t>(sample);
            hash ^= static_cast<std::uint64_t>(bits);
            hash *= 1099511628211ULL;
        }
        for (float sample : right)
        {
            const auto bits = std::bit_cast<std::uint32_t>(sample);
            hash ^= static_cast<std::uint64_t>(bits);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
} // namespace

TEST_CASE("Engine dispatches block MIDI at sub-block sample offsets", "[render][midi]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.05f;
    p.layerA.envelopes[0].sustainLevel = 0.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.05f;

    auto earlyEngineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated, not stack (see docs/TESTING.md)
    auto& earlyEngine = *earlyEngineHolder;
    earlyEngine.prepare(kSampleRate);
    earlyEngine.loadPatch(p);

    auto lateEngineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated, not stack (see docs/TESTING.md)
    auto& lateEngine = *lateEngineHolder;
    lateEngine.prepare(kSampleRate);
    lateEngine.loadPatch(p);

    const int blockSize = 256;
    std::vector<float> earlyL(static_cast<std::size_t>(blockSize));
    std::vector<float> lateL(static_cast<std::size_t>(blockSize));
    std::vector<float> zeros(static_cast<std::size_t>(blockSize), 0.0f);

    render::BlockMidiEvent noteOnEarly{};
    noteOnEarly.sampleOffset = 0;
    noteOnEarly.type = render::BlockMidiType::NoteOn;
    noteOnEarly.note = 60;
    noteOnEarly.channel = 0;
    noteOnEarly.velocity = 100;

    render::BlockMidiEvent noteOnLate = noteOnEarly;
    noteOnLate.sampleOffset = 128;

    core::StereoBlockView earlyView(earlyL.data(), zeros.data(), earlyL.size());
    earlyEngine.process(earlyView, &noteOnEarly, 1);

    core::StereoBlockView lateView(lateL.data(), zeros.data(), lateL.size());
    lateEngine.process(lateView, &noteOnLate, 1);

    float earlyEnergyFirstHalf = 0.0f;
    float lateEnergyFirstHalf = 0.0f;
    for (int i = 0; i < 128; ++i)
    {
        earlyEnergyFirstHalf += std::abs(earlyL[static_cast<std::size_t>(i)]);
        lateEnergyFirstHalf += std::abs(lateL[static_cast<std::size_t>(i)]);
    }

    float lateEnergySecondHalf = 0.0f;
    for (int i = 128; i < blockSize; ++i)
        lateEnergySecondHalf += std::abs(lateL[static_cast<std::size_t>(i)]);

    REQUIRE(earlyEnergyFirstHalf > 1.0f);
    REQUIRE(lateEnergyFirstHalf < earlyEnergyFirstHalf * 0.25f);
    REQUIRE(lateEnergySecondHalf > lateEnergyFirstHalf * 2.0f);
}

TEST_CASE("Golden render fingerprints are deterministic", "[render][golden]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.2f;
    p.layerA.envelopes[0].sustainLevel = 0.8f;
    p.layerA.envelopes[0].releaseSeconds = 0.3f;

    render::BlockMidiEvent noteOn{};
    noteOn.type = render::BlockMidiType::NoteOn;
    noteOn.note = 60;
    noteOn.channel = 0;
    noteOn.velocity = 100;

    auto renderFingerprint = [&]() {
        auto engineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated, not stack (see docs/TESTING.md)
        auto& engine = *engineHolder;
        engine.prepare(kSampleRate);
        engine.loadPatch(p);
        std::vector<float> left(24000, 0.0f);
        std::vector<float> right(24000, 0.0f);
        core::StereoBlockView view(left.data(), right.data(), left.size());
        engine.process(view, &noteOn, 1);
        return fingerprintAudio(left, right);
    };

    const auto fpA = renderFingerprint();
    const auto fpB = renderFingerprint();
    REQUIRE(fpA != 0);
    REQUIRE(fpA == fpB);
}
