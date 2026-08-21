#include <memory>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"

// Regression coverage for a real bug found investigating systematic near-silent
// renders on resonant/physically-modeled taxonomy-batch briefs: the Resonator
// engine (EngineType::Resonator) produced near-zero output when it was a patch's
// only active operator -- confirmed via isolated A/B render (a patch measured
// peak=0.137 with a masking second operator, 0.0003 with that operator muted).
// Root cause was in oscillator::ResonatorOscillator: a fixed ~6ms exciter burst
// regardless of Q (too short to ring up a narrow, high-decay passband) plus no
// real output-level calibration (weightNorm_ only balanced relative mode mix,
// never targeted an absolute output level). Fixed by scaling the exciter burst
// with `decay` and adding a decay-compensated makeup gain -- see
// ResonatorOscillator.hpp's renderSample()/recomputeModeFilters() comments for
// the full derivation. This test locks in "resonator-only patch is actually
// audible across its whole decay range", not just at decay=0.

using namespace pw8;

namespace
{
    constexpr double kSampleRate = 48000.0;

    float renderPeak(render::Engine& engine, int numSamples)
    {
        std::vector<float> left(static_cast<std::size_t>(numSamples));
        std::vector<float> right(static_cast<std::size_t>(numSamples));
        core::StereoBlockView view(left.data(), right.data(), left.size());
        engine.process(view);

        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = std::max(peak, std::fabs(left[static_cast<std::size_t>(i)]));
        return peak;
    }

    // Same silence-gate threshold generate-taxonomy-batch.ts uses to drop a
    // patch as unusable (SILENCE_PEAK_THRESHOLD in that script).
    constexpr float kSilenceGatePeak = 0.01f;
} // namespace

TEST_CASE("Resonator engine alone clears the silence gate across its decay range", "[engine][resonator]")
{
    for (float decay : {0.0f, 0.35f, 0.5f, 0.7f, 0.85f, 1.0f})
    {
        patch::Patch p = patch::Patch::makeInit();
        p.layerA.operators[0].engine = algorithm::EngineType::Resonator;
        p.layerA.operators[0].level = 0.85f;
        p.layerA.operators[0].resonatorStructure = 0.3f;
        p.layerA.operators[0].resonatorDecay = decay;
        p.layerA.operators[0].resonatorBrightness = 0.5f;
        for (std::size_t i = 1; i < p.layerA.operators.size(); ++i)
            p.layerA.operators[i].level = 0.0f; // resonator-only -- no masking operator.
        p.layerA.envelopes[0].attackSeconds = 0.01f;
        p.layerA.envelopes[0].decaySeconds = 0.4f;
        p.layerA.envelopes[0].sustainLevel = 0.8f;
        p.layerA.envelopes[0].releaseSeconds = 1.2f;

        auto engineHolder = std::make_unique<render::Engine>(); // 19MB object -- must be heap-allocated (see docs/TESTING.md)
        auto& engine = *engineHolder;
        engine.prepare(kSampleRate);
        REQUIRE(engine.loadPatch(p));
        engine.noteOn(60, 0, 100);

        const float peak = renderPeak(engine, static_cast<int>(kSampleRate * 1.5)); // 1.5s, well past attack+decay.
        INFO("decay=" << decay << " peak=" << peak);
        CHECK(peak >= kSilenceGatePeak);
        CHECK(peak < 1.0f); // and not blown out by the makeup gain either.
    }
}
