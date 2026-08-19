#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/envelope/SegmentEnvelope.hpp"
#include "pw8/core/Types.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "pw8/patch/PatchSerializer.hpp"

using namespace pw8::patch;
using namespace pw8::envelope;

TEST_CASE("Patch roundtrips through JSON", "[patch][serialization]")
{
    Patch p = Patch::makeInit();
    p.metadata.id = "test-id-123";
    p.metadata.name = "Test Patch";
    p.metadata.category = "bass";
    p.metadata.tags = {"warm", "dark"};
    p.seed = 999;
    p.layerA.operators[0].engine = pw8::algorithm::EngineType::Classic;
    p.layerA.operators[0].classicWaveform = pw8::oscillator::ClassicWaveform::Saw;
    p.layerA.envelopes[0].attackSeconds = 0.05f;
    p.layerA.envelopes[0].sustainLevel = 0.6f;
    p.voiceSettings.polyphony = 8;

    const auto json = savePatchToJson(p);
    REQUIRE_FALSE(json.empty());

    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.metadata.id == "test-id-123");
    REQUIRE(result.patch.metadata.name == "Test Patch");
    REQUIRE(result.patch.metadata.category == "bass");
    REQUIRE(result.patch.metadata.tags.size() == 2);
    REQUIRE(result.patch.seed == 999);
    REQUIRE(result.patch.layerA.operators[0].classicWaveform == pw8::oscillator::ClassicWaveform::Saw);
    REQUIRE(result.patch.layerA.envelopes[0].attackSeconds == Catch::Approx(0.05f));
    REQUIRE(result.patch.layerA.envelopes[0].sustainLevel == Catch::Approx(0.6f));
    REQUIRE(result.patch.voiceSettings.polyphony == 8);
    REQUIRE(result.patch.schemaVersion == pw8::core::kPatchSchemaVersion);
}

TEST_CASE("uiFocus roundtrips through JSON", "[patch][serialization]")
{
    Patch p = Patch::makeInit();
    p.uiFocus.maxKnobs = 3;
    p.uiFocus.knobs.push_back({UiFocusKnobKind::Macro, 0, {}, {}});
    p.uiFocus.knobs.push_back({UiFocusKnobKind::Macro, 1, {}, "Space"});
    p.uiFocus.knobs.push_back({UiFocusKnobKind::Param, 0, "filterCutoffHz", "Brightness"});

    const auto json = savePatchToJson(p);
    REQUIRE_FALSE(json.empty());

    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.uiFocus.maxKnobs == 3);
    REQUIRE(result.patch.uiFocus.knobs.size() == 3);
    REQUIRE(result.patch.uiFocus.knobs[0].kind == UiFocusKnobKind::Macro);
    REQUIRE(result.patch.uiFocus.knobs[0].macroIndex == 0);
    REQUIRE(result.patch.uiFocus.knobs[1].label == "Space");
    REQUIRE(result.patch.uiFocus.knobs[2].paramId == "filterCutoffHz");
    REQUIRE(result.patch.uiFocus.knobs[2].label == "Brightness");
}

TEST_CASE("morphKoin and uiFocus morph kind roundtrip through JSON", "[patch][serialization]")
{
    Patch p = Patch::makeInit();
    p.morphKoin.label = "EVOLVE";
    p.morphKoin.description = "Tight to wide";
    p.morphKoin.defaultPosition = 0.25f;
    p.morphKoin.position = 0.35f;
    p.morphKoin.curve = "smooth";
    p.morphKoin.wrap = false;

    MorphKoinKeyframe kf0;
    kf0.name = "TIGHT";
    kf0.position = 0.0f;
    kf0.hasMacroValues = true;
    kf0.macroValues[0] = 0.1f;
    kf0.paramOverrides["filterCutoffHz"] = MorphParamOverride{800.0f, {}, {}};
    p.morphKoin.keyframes.push_back(kf0);

    MorphKoinKeyframe kf1;
    kf1.name = "WIDE";
    kf1.position = 1.0f;
    kf1.hasMacroValues = true;
    kf1.macroValues[0] = 0.8f;
    p.morphKoin.keyframes.push_back(kf1);

    p.uiFocus.maxKnobs = 3;
    p.uiFocus.knobs.push_back({UiFocusKnobKind::Morph, 0, {}, "EVOLVE"});
    p.uiFocus.knobs.push_back({UiFocusKnobKind::Macro, 0, {}, "BLOOM"});

    const auto json = savePatchToJson(p);
    REQUIRE_FALSE(json.empty());

    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.morphKoin.label == "EVOLVE");
    REQUIRE(result.patch.morphKoin.keyframes.size() == 2);
    REQUIRE(result.patch.morphKoin.keyframes[0].name == "TIGHT");
    REQUIRE(result.patch.morphKoin.keyframes[0].hasMacroValues);
    REQUIRE(result.patch.morphKoin.keyframes[0].macroValues[0] == Catch::Approx(0.1f));
    REQUIRE(result.patch.morphKoin.keyframes[0].paramOverrides.at("filterCutoffHz").value ==
            Catch::Approx(800.0f));
    REQUIRE(result.patch.morphKoin.position == Catch::Approx(0.35f));
    REQUIRE(result.patch.uiFocus.knobs[0].kind == UiFocusKnobKind::Morph);
    REQUIRE(result.patch.uiFocus.knobs[0].label == "EVOLVE");
}

TEST_CASE("morphKoin loads up to sixteen keyframes", "[patch][serialization][morph]")
{
    Patch p = Patch::makeInit();
    p.morphKoin.label = "SCENE";
    for (std::size_t i = 0; i < pw8::core::kMaxMorphKeyframes + 2; ++i)
    {
        MorphKoinKeyframe kf;
        kf.name = "KF" + std::to_string(i);
        kf.position = static_cast<float>(i) / static_cast<float>(pw8::core::kMaxMorphKeyframes);
        p.morphKoin.keyframes.push_back(kf);
    }

    const auto result = loadPatchFromJson(savePatchToJson(p));
    REQUIRE(result.ok);
    REQUIRE(result.patch.morphKoin.keyframes.size() == pw8::core::kMaxMorphKeyframes);
}

TEST_CASE("morphKoin paramOverrides object form roundtrips", "[patch][serialization][morph]")
{
    Patch p = Patch::makeInit();
    p.morphKoin.label = "CURVE";
    p.morphKoin.autoplaySource = "lfo1";

    MorphKoinKeyframe kf0;
    kf0.name = "A";
    kf0.position = 0.0f;
    kf0.color = "#ff8800";
    kf0.paramOverrides["filterCutoffHz"] = MorphParamOverride{800.0f, "smooth", "log"};

    MorphKoinKeyframe kf1;
    kf1.name = "B";
    kf1.position = 1.0f;
    p.morphKoin.keyframes = {kf0, kf1};

    const auto result = loadPatchFromJson(savePatchToJson(p));
    REQUIRE(result.ok);
    REQUIRE(result.patch.morphKoin.autoplaySource == "lfo1");
    const auto& ov = result.patch.morphKoin.keyframes[0].paramOverrides.at("filterCutoffHz");
    REQUIRE(ov.value == Catch::Approx(800.0f));
    REQUIRE(ov.easing == "smooth");
    REQUIRE(ov.response == "log");
    REQUIRE(result.patch.morphKoin.keyframes[0].color == "#ff8800");
}

TEST_CASE("voiceSettings morphDissemination roundtrips", "[patch][serialization][morph]")
{
    Patch p = Patch::makeInit();
    p.voiceSettings.morphDissemination = true;
    const auto result = loadPatchFromJson(savePatchToJson(p));
    REQUIRE(result.ok);
    REQUIRE(result.patch.voiceSettings.morphDissemination);
}

TEST_CASE("A v1 document's singular ampEnvelope/lfo1 migrate into envelopes[0]/lfos[0]",
          "[patch][serialization][migration]")
{
    // Hand-written schema v1 document (as any real patch saved before this pass
    // would be) -- proves migrateToCurrentSchema() actually runs on load, not just
    // that fromJson() can parse the new array shape.
    constexpr auto v1Json = R"({
        "schemaVersion": 1,
        "layerA": {
            "ampEnvelope": {"attackSeconds": 0.09, "decaySeconds": 0.2, "sustainLevel": 0.55, "releaseSeconds": 0.3},
            "lfo1": {"waveform": 2, "mode": 1, "rateHz": 3.5, "syncDivisionIndex": 5, "phaseOffset": 0.25}
        }
    })";

    const auto result = loadPatchFromJson(v1Json);
    REQUIRE(result.ok);
    REQUIRE(result.originalSchemaVersion == 1);
    REQUIRE(result.patch.schemaVersion == pw8::core::kPatchSchemaVersion); // migrated up to current (v3).

    REQUIRE(result.patch.layerA.envelopes[0].attackSeconds == Catch::Approx(0.09f));
    REQUIRE(result.patch.layerA.envelopes[0].sustainLevel == Catch::Approx(0.55f));
    // Envelopes 2-8 weren't in the v1 document -- migration doesn't invent data for them.
    REQUIRE(result.patch.layerA.envelopes[1].attackSeconds == Catch::Approx(0.005f)); // struct default.

    REQUIRE(result.patch.layerA.lfos[0].rateHz == Catch::Approx(3.5f));
    REQUIRE(result.patch.layerA.lfos[0].syncDivisionIndex == 5);
    REQUIRE(result.patch.layerA.lfos[1].rateHz == Catch::Approx(2.0f)); // struct default.

    // Re-saving now writes the current (v3) array shape, not the old singular fields.
    const auto resaved = savePatchToJson(result.patch);
    REQUIRE(resaved.find("\"envelopes\"") != std::string::npos);
    REQUIRE(resaved.find("\"lfos\"") != std::string::npos);
}

TEST_CASE("A v1 document's modRoutes source ordinals remap to the new ModSource enum, not silently reinterpreted",
          "[patch][serialization][migration]")
{
    // Real bug, caught before shipping (docs/ROADMAP.md "GATE 5"): inserting
    // Lfo2-8/Env1-8 into ModSource between Lfo1 and Velocity shifted every
    // enum ordinal after Lfo1. A v1 preset's "source": 2 meant AmpEnvelope under
    // the old 15-value enum; under the new 29-value enum, ordinal 2 means Lfo2 --
    // silently the WRONG source, not a load failure. This is exactly the shape
    // dark-bass.pw8 and wide-saw.pw8 (real shipped presets) are in.
    constexpr auto v1Json = R"({
        "schemaVersion": 1,
        "layerA": {
            "modRoutes": [
                {"source": 1,  "destination": 1, "targetIndex": 0, "amount": 10.0, "scope": 0},
                {"source": 2,  "destination": 1, "targetIndex": 0, "amount": 20.0, "scope": 0},
                {"source": 3,  "destination": 1, "targetIndex": 0, "amount": 30.0, "scope": 0},
                {"source": 7,  "destination": 3, "targetIndex": 0, "amount": 1.0,  "scope": 0},
                {"source": 14, "destination": 3, "targetIndex": 0, "amount": 1.0,  "scope": 0}
            ]
        }
    })";

    const auto result = loadPatchFromJson(v1Json);
    REQUIRE(result.ok);
    const auto& routes = result.patch.layerA.modRoutes;
    REQUIRE(routes.size() == 5);

    REQUIRE(routes[0].source == pw8::modulation::ModSource::Lfo1);   // old 1 (Lfo1) -> new Lfo1 (unchanged).
    REQUIRE(routes[1].source == pw8::modulation::ModSource::Env1);   // old 2 (AmpEnvelope) -> new Env1.
    REQUIRE(routes[2].source == pw8::modulation::ModSource::Velocity); // old 3 (Velocity) -> new Velocity (17).
    REQUIRE(routes[3].source == pw8::modulation::ModSource::Macro1); // old 7 (Macro1) -> new Macro1 (21).
    REQUIRE(routes[4].source == pw8::modulation::ModSource::Macro8); // old 14 (Macro8) -> new Macro8 (28).
}

TEST_CASE("loadPatchFromJson preserves Expression mod source ordinal 30", "[patch][serialization][mod]")
{
    constexpr auto json = R"({
        "schemaVersion": 3,
        "layerA": {
            "modRoutes": [
                {"source": 30, "destination": 1, "targetIndex": 0, "amount": 12.0, "scope": 0}
            ]
        }
    })";

    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.layerA.modRoutes.size() >= 1);
    REQUIRE(result.patch.layerA.modRoutes[0].source == pw8::modulation::ModSource::Expression);
}

TEST_CASE("Patch algorithm graph roundtrips through JSON", "[patch][serialization][algorithm]")
{
    Patch p = Patch::makeInit();
    p.layerA.algorithm.edges.push_back(
        pw8::algorithm::AlgorithmEdge{pw8::core::NodeId(1), pw8::core::NodeId(0), pw8::algorithm::EdgeType::PhaseMod, 0.75f});

    const auto json = savePatchToJson(p);
    const auto result = loadPatchFromJson(json);
    REQUIRE(result.ok);
    REQUIRE(result.patch.layerA.algorithm.nodes.size() == pw8::core::kNodesPerLayer);

    bool foundEdge = false;
    for (const auto& e : result.patch.layerA.algorithm.edges)
    {
        if (e.source.get() == 1 && e.destination.get() == 0 && e.type == pw8::algorithm::EdgeType::PhaseMod)
        {
            foundEdge = true;
            REQUIRE(e.amount == Catch::Approx(0.75f));
        }
    }
    REQUIRE(foundEdge);
}

TEST_CASE("loadPatchFromJson rejects malformed input", "[patch][serialization][robustness]")
{
    const auto result = loadPatchFromJson("{not valid json");
    REQUIRE_FALSE(result.ok);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("loadPatchFromJson rejects a non-object root", "[patch][serialization][robustness]")
{
    const auto result = loadPatchFromJson("[1, 2, 3]");
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("loadPatchFromJson fills in sane defaults for a minimal document", "[patch][serialization]")
{
    const auto result = loadPatchFromJson(R"({"schemaVersion": 1})");
    REQUIRE(result.ok);
    REQUIRE(result.patch.layerA.operators.size() == pw8::core::kNodesPerLayer);
    REQUIRE(result.patch.voiceSettings.polyphony >= 1);
}

TEST_CASE("A v2 document without warp fields defaults wt warps on migration to v3", "[patch][serialization][migration]")
{
    constexpr auto v2Json = R"({
        "schemaVersion": 2,
        "layerA": {
            "operators": [
                {"engine": 1, "level": 1.0}
            ]
        }
    })";

    const auto result = loadPatchFromJson(v2Json);
    REQUIRE(result.ok);
    REQUIRE(result.originalSchemaVersion == 2);
    REQUIRE(result.patch.schemaVersion == pw8::core::kPatchSchemaVersion);

    const auto& op = result.patch.layerA.operators[0];
    REQUIRE(op.wtBend == Catch::Approx(0.0f));
    REQUIRE(op.wtAsymmetry == Catch::Approx(0.0f));
    REQUIRE(op.wtSyncRatio == Catch::Approx(1.0f));
    REQUIRE(op.wtSyncAmount == Catch::Approx(0.0f));
    REQUIRE(op.wtFormantShift == Catch::Approx(0.0f));
}

TEST_CASE("filter modeMorph roundtrips and migrates from legacy mode", "[patch][serialization][blades]")
{
    Patch p = Patch::makeInit();
    p.layerA.filter1.enabled = true;
    p.layerA.filter1.mode = pw8::filter::FilterMode::Bandpass;
    p.layerA.filter1.modeMorph = 0.35f;

    const auto json = savePatchToJson(p);
    const auto loaded = loadPatchFromJson(json);
    REQUIRE(loaded.ok);
    REQUIRE(loaded.patch.layerA.filter1.modeMorph == Catch::Approx(0.35f));

    const auto legacy = loadPatchFromJson(R"({"layerA":{"filter1":{"enabled":true,"mode":2,"cutoffHz":1200}}})");
    REQUIRE(legacy.ok);
    REQUIRE(legacy.patch.layerA.filter1.modeMorph == Catch::Approx(0.5f));
}

TEST_CASE("filter routing and F2 cutoff offset roundtrip", "[patch][serialization][blades][routing]")
{
    Patch p = Patch::makeInit();
    p.layerA.filterRouting = 0.65f;
    p.layerA.filter2.cutoffOffsetSemitones = 7.0f;

    const auto loaded = loadPatchFromJson(savePatchToJson(p));
    REQUIRE(loaded.ok);
    REQUIRE(loaded.patch.layerA.filterRouting == Catch::Approx(0.65f));
    REQUIRE(loaded.patch.layerA.filter2.cutoffOffsetSemitones == Catch::Approx(7.0f));
}

TEST_CASE("masterDynamics roundtrips through JSON", "[patch][serialization][dynamics]")
{
    Patch p = Patch::makeInit();
    p.masterDynamics.enabled = true;
    p.masterDynamics.mode = MasterDynamicsMode::Follower;
    p.masterDynamics.thresholdDb = -18.0f;
    p.masterDynamics.ratio = 6.0f;
    p.masterDynamics.attackMs = 3.0f;
    p.masterDynamics.releaseMs = 120.0f;
    p.masterDynamics.sidechainGain = 0.85f;
    p.masterDynamics.vactrolSlewMs = 55.0f;
    p.masterDynamics.makeupDb = 4.0f;
    p.masterDynamics.mix = 0.75f;

    const auto loaded = loadPatchFromJson(savePatchToJson(p));
    REQUIRE(loaded.ok);
    REQUIRE(loaded.patch.masterDynamics.enabled);
    REQUIRE(loaded.patch.masterDynamics.mode == MasterDynamicsMode::Follower);
    REQUIRE(loaded.patch.masterDynamics.thresholdDb == Catch::Approx(-18.0f));
    REQUIRE(loaded.patch.masterDynamics.ratio == Catch::Approx(6.0f));
    REQUIRE(loaded.patch.masterDynamics.attackMs == Catch::Approx(3.0f));
    REQUIRE(loaded.patch.masterDynamics.releaseMs == Catch::Approx(120.0f));
    REQUIRE(loaded.patch.masterDynamics.sidechainGain == Catch::Approx(0.85f));
    REQUIRE(loaded.patch.masterDynamics.vactrolSlewMs == Catch::Approx(55.0f));
    REQUIRE(loaded.patch.masterDynamics.makeupDb == Catch::Approx(4.0f));
    REQUIRE(loaded.patch.masterDynamics.mix == Catch::Approx(0.75f));
}

TEST_CASE("legacy patch without masterDynamics defaults to bypass", "[patch][serialization][dynamics]")
{
    const auto result = loadPatchFromJson(R"({"schemaVersion":3,"metadata":{"name":"Init"}})");
    REQUIRE(result.ok);
    REQUIRE_FALSE(result.patch.masterDynamics.enabled);
    REQUIRE(result.patch.masterDynamics.mode == MasterDynamicsMode::Envelope);
}

TEST_CASE("segment envelope chain roundtrips through JSON", "[patch][serialization][segment]")
{
    Patch p = Patch::makeInit();
    auto& chain = p.layerA.segmentEnvelopeChains[0];
    chain.segments[0] = {SegmentType::Ramp, 120.0f, 1.0f, "inQuartic"};
    chain.segments[1] = {SegmentType::Hold, 400.0f, 1.0f, ""};
    chain.segments[2] = {SegmentType::Ramp, 800.0f, 0.0f, "sine"};
    chain.segmentCount = 3;
    chain.loopStart = 0;
    chain.loopEnd = 2;

    const auto loaded = loadPatchFromJson(savePatchToJson(p));
    REQUIRE(loaded.ok);
    const auto& out = loaded.patch.layerA.segmentEnvelopeChains[0];
    REQUIRE(out.segmentCount == 3);
    REQUIRE(out.segments[0].type == SegmentType::Ramp);
    REQUIRE(out.segments[0].durationMs == Catch::Approx(120.0f));
    REQUIRE(out.segments[0].level == Catch::Approx(1.0f));
    REQUIRE(out.segments[0].shape == "inQuartic");
    REQUIRE(out.segments[1].type == SegmentType::Hold);
    REQUIRE(out.segments[2].shape == "sine");
    REQUIRE(out.loopStart == 0);
    REQUIRE(out.loopEnd == 2);
}

TEST_CASE("legacy patch without segment chains uses ADSR fallback", "[patch][serialization][segment]")
{
    const auto result = loadPatchFromJson(R"({"schemaVersion":3,"metadata":{"name":"Init"}})");
    REQUIRE(result.ok);
    for (const auto& chain : result.patch.layerA.segmentEnvelopeChains)
        REQUIRE_FALSE(chain.isActive());
}
