#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"
#include "pw8/sequencer/ArpeggiatorTypes.hpp"

using namespace pw8;

TEST_CASE("Engine::process realtime smoke — many blocks without allocation failures", "[realtime][engine]")
{
    render::Engine engine;
    engine.prepare(48000.0);
    auto patch = patch::Patch::makeInit();
    patch.layerA.operators[0].level = 0.8f;
    REQUIRE(engine.loadPatch(patch));

    std::array<float, 512> left{};
    std::array<float, 512> right{};
    core::StereoBlockView block(left.data(), right.data(), 512);

    engine.noteOn(60, 0, 100);
    for (int blockIdx = 0; blockIdx < 256; ++blockIdx)
    {
        engine.process(block);
        for (std::size_t i = 0; i < 512; ++i)
        {
            REQUIRE(std::isfinite(left[i]));
            REQUIRE(std::isfinite(right[i]));
        }
    }
    engine.noteOff(60, 0, 0);
    engine.allNotesOff();
}

TEST_CASE("Engine allSoundOff silences arpeggiator voices immediately", "[realtime][engine][midi]")
{
    render::Engine engine;
    engine.prepare(48000.0);
    auto patch = patch::Patch::makeInit();
    patch.arpeggiator.enabled = true;
    patch.arpeggiator.rateMode = sequencer::ArpRateMode::TempoSync;
    patch.arpeggiator.syncDivisionIndex = 5;
    patch.arpeggiator.numSteps = 4;
    REQUIRE(engine.loadPatch(patch));

    std::array<float, 512> left{};
    std::array<float, 512> right{};
    core::StereoBlockView block(left.data(), right.data(), 512);

    engine.noteOn(60, 0, 100);
    for (int i = 0; i < 48; ++i)
        engine.process(block);

    engine.allSoundOff();

    float peak = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        engine.process(block);
        for (float s : left)
            peak = std::max(peak, std::abs(s));
        for (float s : right)
            peak = std::max(peak, std::abs(s));
    }
    REQUIRE(peak < 1.0e-4f);
}
