#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"

// Proves the rest of Engine's "Live parameter API" (docs/PLUGIN_ARCHITECTURE.md
// "Automation") behaves the same way EngineMacroLiveUpdateTests.cpp already proved
// for macros: filter/operator/effect changes reach a currently-sustaining voice
// immediately, and an arpeggiator's scalar-only live update doesn't reset its
// held-note/pattern state the way a full configure() would.

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

TEST_CASE("Engine::setFilterLive audibly changes a currently-held voice", "[engine][liveparams]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw; // harmonically rich -- lowpass has something to remove.
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));
    engine.noteOn(60, 0, 100);

    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsOpen = renderRms(engine, 1000); // no filter yet -- full brightness.

    filter::FilterParams closed;
    closed.enabled = true;
    closed.mode = filter::FilterMode::Lowpass;
    closed.cutoffHz = 150.0f; // well below the saw's fundamental+harmonics at note 60.
    closed.resonance = 0.1f;
    engine.setFilterLive(closed); // mid-hold, no re-trigger.

    const float rmsClosed = renderRms(engine, 1000);
    REQUIRE(rmsClosed < rmsOpen * 0.5f); // the SAME still-held voice audibly darkened.
}

TEST_CASE("Engine::setOperatorLive changes a currently-held voice's level", "[engine][liveparams]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));
    engine.noteOn(60, 0, 100);

    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsFull = renderRms(engine, 1000);
    REQUIRE(rmsFull > 0.1f);

    auto params = engine.getOperatorParams(0);
    params.level = 0.0f;
    engine.setOperatorLive(0, params);

    const float rmsMuted = renderRms(engine, 1000);
    REQUIRE(rmsMuted < 0.01f);
}

TEST_CASE("Engine::setModRoutesLive audibly changes a currently-held voice, not just the next note-on",
          "[engine][liveparams]")
{
    // The property drag-to-modulate (docs/UI.md) depends on directly: unlike
    // setEnvelopeLive() (documented next-note-on-only), a mod route added mid-hold
    // must reach an already-sustaining voice the very next sample.
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw; // harmonically rich.
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.filter1.enabled = true;
    p.layerA.filter1.mode = filter::FilterMode::Lowpass;
    p.layerA.filter1.cutoffHz = 12000.0f; // wide open -- no route yet.
    p.layerA.filter1.resonance = 0.1f;

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));
    engine.noteOn(60, 0, 100); // velocity7=100 -> unit velocity ~0.787.

    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsBeforeRoute = renderRms(engine, 1000);

    // Velocity -> FilterCutoff, a large negative semitone offset per unit velocity:
    // at ~0.787 unit velocity (-100 * 0.787 =~ -78.7 semitones, a ~6.6-octave drop)
    // this pulls the 12kHz cutoff down to ~127Hz -- comfortably below note 60's
    // ~261.6Hz fundamental, so the reduction is dramatic and unambiguous rather than
    // just "fewer high harmonics" (an earlier, gentler amount left the fundamental
    // and first few harmonics passing, which didn't move RMS nearly as much).
    core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> routes;
    modulation::ModRoute route;
    route.source = modulation::ModSource::Velocity;
    route.destination = modulation::ModDestination::FilterCutoff;
    route.amount = -100.0f;
    routes.push_back(route);
    engine.setModRoutesLive(routes); // mid-hold, no re-trigger.

    const float rmsAfterRoute = renderRms(engine, 1000);
    REQUIRE(rmsAfterRoute < rmsBeforeRoute * 0.5f); // the SAME still-held voice audibly darkened.

    // And removing the route (an empty list) brings it back open on the same voice.
    engine.setModRoutesLive({});
    const float rmsAfterRemoval = renderRms(engine, 1000);
    REQUIRE(rmsAfterRemoval > rmsAfterRoute * 1.5f);
}

TEST_CASE("Engine::setInsertEffectLive changes a currently-held voice's output", "[engine][liveparams]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.operators[0].level = 1.0f;
    p.layerA.gain = 2.0f; // deliberately loud, so saturation has something to bite into.
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));
    engine.noteOn(60, 0, 127);

    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsDry = renderRms(engine, 1000);

    effects::EffectSlotParams sat;
    sat.type = effects::EffectType::Saturation;
    sat.mix = 1.0f;
    sat.saturationDriveDb = 24.0f;
    engine.setInsertEffectLive(0, sat); // mid-hold, layer A's first insert slot.

    const float rmsSaturated = renderRms(engine, 1000);
    // Saturation compresses a loud signal toward unity -- RMS should measurably drop
    // relative to the unprocessed loud dry signal.
    REQUIRE(rmsSaturated < rmsDry);
}

TEST_CASE("Engine::setArpeggiatorScalarLive preserves held notes and pattern state, unlike configure()",
          "[engine][liveparams][arpeggiator]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.02f;
    p.layerA.envelopes[0].sustainLevel = 0.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.01f;

    p.arpeggiator.enabled = true;
    p.arpeggiator.mode = sequencer::ArpMode::Up;
    p.arpeggiator.rateMode = sequencer::ArpRateMode::Free;
    p.arpeggiator.rateHz = 20.0f;
    p.arpeggiator.numSteps = 1;
    p.arpeggiator.steps[0] = sequencer::ArpStep{};

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));

    engine.noteOn(60, 0, 100);
    engine.noteOn(64, 0, 100);

    // Run long enough for several arp-triggered onsets at 20Hz.
    std::vector<float> beforeL(4800), beforeR(4800);
    core::StereoBlockView beforeView(beforeL.data(), beforeR.data(), beforeL.size());
    engine.process(beforeView);
    bool anyOnsetBefore = false;
    for (float v : beforeL)
        if (std::abs(v) > 0.05f)
            anyOnsetBefore = true;
    REQUIRE(anyOnsetBefore);

    // Live scalar-only update -- a different rate, but no noteOn/noteOff/configure()
    // call. If this reset held-note state the way configure() does, the arp would go
    // silent from here on (no notes "held" from its point of view).
    auto liveParams = engine.getArpeggiatorParams();
    liveParams.rateHz = 35.0f;
    engine.setArpeggiatorScalarLive(liveParams);

    std::vector<float> afterL(4800), afterR(4800);
    core::StereoBlockView afterView(afterL.data(), afterR.data(), afterL.size());
    engine.process(afterView);
    bool anyOnsetAfter = false;
    for (float v : afterL)
        if (std::abs(v) > 0.05f)
            anyOnsetAfter = true;
    REQUIRE(anyOnsetAfter); // still playing -- held notes/pattern position survived the live update.
}

TEST_CASE("Engine::setFilterRoutingLive morphs dual-filter output on held voice", "[engine][liveparams][blades][routing]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    p.layerA.envelopes[0].attackSeconds = 0.001f;
    p.layerA.envelopes[0].decaySeconds = 0.01f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.filter1.enabled = true;
    p.layerA.filter1.cutoffHz = 800.0f;
    p.layerA.filter1.resonance = 0.35f;
    p.layerA.filter2.enabled = true;
    p.layerA.filter2.cutoffHz = 2000.0f;
    p.layerA.filter2.resonance = 0.25f;
    p.layerA.filter2.drive = 0.2f;
    p.layerA.filterRouting = 0.0f;

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(p));
    engine.noteOn(60, 0, 100);

    std::vector<float> settleL(500), settleR(500);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsSerial = renderRms(engine, 2000);
    engine.setFilterRoutingLive(0.5f);
    const float rmsParallel = renderRms(engine, 2000);
    engine.setFilterRoutingLive(1.0f);
    const float rmsCrossfade = renderRms(engine, 2000);

    REQUIRE(rmsSerial > 0.001f);
    REQUIRE(std::abs(rmsParallel - rmsSerial) > 1.0e-4f);
    REQUIRE(std::abs(rmsCrossfade - rmsParallel) > 1.0e-4f);
}
