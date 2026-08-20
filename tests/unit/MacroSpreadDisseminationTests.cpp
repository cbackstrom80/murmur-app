#include <memory>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/modulation/MacroSpread.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"

using namespace pw8;

namespace
{
    constexpr double kSampleRate = 48000.0;

    [[nodiscard]] float renderRms(render::Engine& engine, int numSamples)
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

TEST_CASE("spreadSummaryForMacro lists deduplicated destination short names", "[mod][spread][koin]")
{
    std::vector<modulation::ModRoute> routes;
    routes.push_back(
        {modulation::ModSource::Macro1, modulation::ModDestination::FilterCutoff, 0, 12.0f, modulation::ModScope::Voice});
    routes.push_back(
        {modulation::ModSource::Macro1, modulation::ModDestination::FilterResonance, 0, 0.2f, modulation::ModScope::Voice});
    routes.push_back({modulation::ModSource::Macro1, modulation::ModDestination::OperatorWavetablePosition, 1, 0.35f,
                      modulation::ModScope::Voice});
    routes.push_back(
        {modulation::ModSource::Macro2, modulation::ModDestination::Pan, 0, 0.4f, modulation::ModScope::Voice});

    REQUIRE(modulation::spreadSummaryForMacro(0, routes) == "Filter, Reso, WT");
    REQUIRE(modulation::spreadDestinationCountForMacro(0, routes) == 3);
    REQUIRE(modulation::spreadSummaryForMacro(1, routes) == "Pan");
    REQUIRE(modulation::spreadSummaryForMacro(2, routes).empty());
}

TEST_CASE("macroDissemination freezes held-voice macro values", "[engine][macro][dissemination]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.voiceSettings.macroDissemination = true;
    p.voiceSettings.disseminationDepth = 0.25f;
    p.voiceSettings.polyphony = 4;
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.05f;

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

    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsBefore = renderRms(engine, 1000);
    REQUIRE(rmsBefore > 0.1f);

    engine.setMacroValue(0, 1.0f);
    REQUIRE(engine.getMacroValue(0) == 1.0f);

    const float rmsAfter = renderRms(engine, 1000);
    REQUIRE(std::abs(rmsAfter - rmsBefore) < 0.02f);

    engine.noteOff(60, 0, 0);
    std::vector<float> releaseL(8000), releaseR(8000);
    core::StereoBlockView releaseView(releaseL.data(), releaseR.data(), releaseL.size());
    engine.process(releaseView);

    engine.noteOn(62, 0, 100);
    std::vector<float> settle2L(500), settle2R(500);
    core::StereoBlockView settle2View(settle2L.data(), settle2R.data(), settle2L.size());
    engine.process(settle2View);

    const float rmsNewNote = renderRms(engine, 1000);
    REQUIRE(rmsNewNote < rmsBefore * 0.35f);
}

TEST_CASE("macroDissemination gives different macro samples per voice", "[engine][macro][dissemination]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.voiceSettings.macroDissemination = true;
    p.voiceSettings.disseminationDepth = 0.25f;
    p.voiceSettings.polyphony = 8;
    p.macros[0].value = 0.5f;
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.2f;

    modulation::ModRoute route;
    route.source = modulation::ModSource::Macro1;
    route.destination = modulation::ModDestination::OperatorLevel;
    route.targetIndex = 0;
    route.amount = -0.5f;
    p.layerA.modRoutes.push_back(route);

    auto engineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated, not stack (see docs/TESTING.md)
    auto& engine = *engineHolder;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));

    engine.noteOn(60, 0, 100);
    engine.noteOn(64, 0, 100);

    std::vector<float> settleL(2000), settleR(2000);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsChord = renderRms(engine, 2000);
    REQUIRE(rmsChord > 0.05f);

    engine.setMacroValue(0, 0.0f);
    const float rmsFrozen = renderRms(engine, 2000);
    REQUIRE(std::abs(rmsFrozen - rmsChord) < 0.02f);
}

TEST_CASE("disseminationDepth gives different timbre per retriggered note", "[engine][macro][dissemination]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.voiceSettings.macroDissemination = true;
    p.voiceSettings.disseminationDepth = 0.25f;
    p.macros[0].value = 0.5f;
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.05f;

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

    const auto measureNote = [&](int note) {
        engine.noteOn(note, 0, 100);
        std::vector<float> settleL(500), settleR(500);
        core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
        engine.process(settleView);
        const float rms = renderRms(engine, 1000);
        engine.noteOff(note, 0, 0);
        std::vector<float> releaseL(4000), releaseR(4000);
        core::StereoBlockView releaseView(releaseL.data(), releaseR.data(), releaseL.size());
        engine.process(releaseView);
        return rms;
    };

    const float rmsFirst = measureNote(60);
    const float rmsSecond = measureNote(60);
    REQUIRE(rmsFirst > 0.05f);
    REQUIRE(rmsSecond > 0.05f);
    REQUIRE(std::abs(rmsFirst - rmsSecond) > 0.025f);

    p.voiceSettings.macroDissemination = false;
    auto uniformEngineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated, not stack (see docs/TESTING.md)
    auto& uniformEngine = *uniformEngineHolder;
    uniformEngine.prepare(kSampleRate);
    REQUIRE(uniformEngine.loadPatch(p));

    const float uniformFirst = measureNote(60);
    const float uniformSecond = measureNote(60);
    REQUIRE(std::abs(uniformFirst - uniformSecond) < 0.03f);
}
