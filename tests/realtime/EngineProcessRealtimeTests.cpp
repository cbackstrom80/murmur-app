#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"

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
