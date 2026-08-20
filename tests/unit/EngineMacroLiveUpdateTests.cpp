#include <memory>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"

// Proves Engine::setMacroValue() actually reaches a currently-sustaining voice's
// per-sample rendering, not just the next note-on -- the property a DAW automating
// a macro mid-hold depends on (see docs/PLUGIN_ARCHITECTURE.md "Automation").

using namespace pw8;

namespace
{
    constexpr double kSampleRate = 48000.0;

    float renderRms(render::Engine& engine, int numSamples)
    {
        std::vector<float> left(static_cast<std::size_t>(numSamples));
        std::vector<float> right(static_cast<std::size_t>(numSamples));
        core::StereoBlockView view(left.data(), right.data(), left.size());
        engine.process(view);

        double sumSq = 0.0;
        for (int i = 0; i < numSamples; ++i)
            sumSq += static_cast<double>(left[static_cast<std::size_t>(i)]) * left[static_cast<std::size_t>(i)];
        return static_cast<float>(std::sqrt(sumSq / numSamples));
    }
} // namespace

TEST_CASE("Engine::setMacroValue changes a currently-held voice's output immediately", "[engine][macro][live]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.05f;

    // Macro1 -> OperatorLevel(0), amount=-1: level *= (1 + macro*(-1)) -- macro=0
    // keeps full level, macro=1 mutes the operator entirely. See ModMatrixExecutor.
    modulation::ModRoute route;
    route.source = modulation::ModSource::Macro1;
    route.destination = modulation::ModDestination::OperatorLevel;
    route.targetIndex = 0;
    route.amount = -1.0f;
    p.layerA.modRoutes.push_back(route);

    auto engineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated, not stack (see docs/TESTING.md)
    auto& engine = *engineHolder;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));

    engine.noteOn(60, 0, 100);

    // Skip the short attack/decay so we're measuring steady sustain, not the envelope ramp.
    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    REQUIRE(engine.getMacroValue(0) == 0.0f); // untouched macro defaults to 0.

    const float rmsBefore = renderRms(engine, 1000);
    REQUIRE(rmsBefore > 0.1f); // full level, clearly audible.

    engine.setMacroValue(0, 1.0f); // mute, mid-hold -- no note-off, no re-trigger.
    REQUIRE(engine.getMacroValue(0) == 1.0f);

    const float rmsAfter = renderRms(engine, 1000);
    REQUIRE(rmsAfter < 0.01f); // the SAME still-held voice is now (near-)silent.

    engine.setMacroValue(0, 0.0f); // and back -- proves it's a live, reversible control, not a one-shot latch.
    const float rmsRestored = renderRms(engine, 1000);
    REQUIRE(rmsRestored > 0.1f);
}
