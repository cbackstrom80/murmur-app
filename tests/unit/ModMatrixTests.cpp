#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "pw8/core/Types.hpp"
#include "pw8/modulation/ModCurveShaping.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"

using namespace pw8::modulation;

TEST_CASE("ModMatrixExecutor routes Velocity to OperatorFilterCutoff with targetIndex", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Velocity, ModDestination::OperatorFilterCutoff, 3, 12.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.velocity = 0.5f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorFilterCutoffSemitones[3] == Catch::Approx(6.0f));
    REQUIRE(out.operatorFilterCutoffSemitones[0] == 0.0f);
}

TEST_CASE("ModMatrixExecutor with no routes produces neutral output", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    ModSourceValues sources;
    sources.velocity = 0.8f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == 0.0f);
    REQUIRE(out.filterResonanceOffset == 0.0f);
    REQUIRE(out.panOffset == 0.0f);
    for (auto m : out.operatorLevelMultiplier)
        REQUIRE(m == 1.0f);
}

TEST_CASE("ModMatrixExecutor routes Velocity to FilterCutoff correctly", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Velocity, ModDestination::FilterCutoff, 0, 12.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.velocity = 0.5f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == Catch::Approx(6.0f)); // 0.5 * 12.
}

TEST_CASE("ModMatrixExecutor composes multiple OperatorLevel routes multiplicatively", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo1, ModDestination::OperatorLevel, 2, 0.5f, ModScope::Voice});
    routes.push_back(ModRoute{ModSource::Env1, ModDestination::OperatorLevel, 2, 0.5f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[0] = 1.0f;
    sources.envelopes[0] = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    // node 2: (1 + 1.0*0.5) * (1 + 1.0*0.5) = 1.5 * 1.5 = 2.25.
    REQUIRE(out.operatorLevelMultiplier[2] == Catch::Approx(2.25f));
    // Untouched operators stay at 1.0.
    REQUIRE(out.operatorLevelMultiplier[0] == 1.0f);
}

TEST_CASE("ModMatrixExecutor reads any of the 8 LFOs/envelopes by index", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo5, ModDestination::FilterCutoff, 0, 10.0f, ModScope::Voice});
    routes.push_back(ModRoute{ModSource::Env7, ModDestination::FilterResonance, 0, 0.4f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[4] = 0.5f; // Lfo5 == index 4.
    sources.envelopes[6] = 0.5f; // Env7 == index 6.

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == Catch::Approx(5.0f));
    REQUIRE(out.filterResonanceOffset == Catch::Approx(0.2f));
}

TEST_CASE("ModMatrixExecutor LFO sources read the shared layer-wide tick at Layer/Global scope, not the "
          "per-voice one",
          "[modulation][scope]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo2, ModDestination::Pan, 0, 1.0f, ModScope::Voice});
    routes.push_back(ModRoute{ModSource::Lfo2, ModDestination::FilterResonance, 0, 1.0f, ModScope::Layer});
    routes.push_back(ModRoute{ModSource::Lfo2, ModDestination::FilterCutoff, 0, 1.0f, ModScope::Global});

    ModSourceValues sources;
    sources.voiceLfos[1] = 0.3f; // this voice's own, independently-phased Lfo2.
    sources.layerLfos[1] = 0.9f; // the shared, layer-wide tick for the same LFO index.

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.panOffset == Catch::Approx(0.3f));            // Voice scope -> per-voice value.
    REQUIRE(out.filterResonanceOffset == Catch::Approx(0.9f)); // Layer scope -> shared value.
    REQUIRE(out.filterCutoffSemitones == Catch::Approx(0.9f)); // Global scope -> shared value too (see ModScope doc).
}

TEST_CASE("ModMatrixExecutor envelope sources ignore declared scope -- always the per-voice value",
          "[modulation][scope]")
{
    // Envelope LAYER/GLOBAL scope is intentionally not implemented (no single
    // coherent trigger point for a layer-wide envelope) -- see ModScope's doc
    // comment. A Layer-scoped Env route must still read the per-voice envelope.
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Env3, ModDestination::Pan, 0, 1.0f, ModScope::Layer});

    ModSourceValues sources;
    sources.envelopes[2] = 0.42f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.panOffset == Catch::Approx(0.42f));
}

TEST_CASE("ModMatrixExecutor skips inactive routes", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::None, ModDestination::FilterCutoff, 0, 999.0f, ModScope::Voice});
    routes.push_back(ModRoute{ModSource::Velocity, ModDestination::None, 0, 999.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.velocity = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == 0.0f);
}

TEST_CASE("ModMatrixExecutor routes ModWheel to FilterCutoff correctly", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::ModWheel, ModDestination::FilterCutoff, 0, 24.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.modWheel = 0.5f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == Catch::Approx(12.0f));
}

TEST_CASE("ModMatrixExecutor routes Expression to FilterResonance correctly", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Expression, ModDestination::FilterResonance, 0, 0.35f, ModScope::Voice});

    ModSourceValues sources;
    sources.expression = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterResonanceOffset == Catch::Approx(0.35f));
}

TEST_CASE("ModMatrixExecutor routes LFO to OperatorWavetableBend with targetIndex", "[modulation][warp]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo1, ModDestination::OperatorWavetableBend, 2, 0.5f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[0] = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorWavetableBendOffset[2] == Catch::Approx(0.5f));
    REQUIRE(out.operatorWavetableBendOffset[0] == 0.0f);
}

TEST_CASE("ModMatrixExecutor routes LFO to OperatorWavetableAsymmetry with targetIndex", "[modulation][warp]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo2, ModDestination::OperatorWavetableAsymmetry, 1, 0.4f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[1] = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorWavetableAsymmetryOffset[1] == Catch::Approx(0.4f));
    REQUIRE(out.operatorWavetableAsymmetryOffset[0] == 0.0f);
}

TEST_CASE("ModMatrixExecutor routes LFO to OperatorWavetableSyncRatio with targetIndex", "[modulation][warp]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo3, ModDestination::OperatorWavetableSyncRatio, 0, 3.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[2] = 0.5f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorWavetableSyncRatioOffset[0] == Catch::Approx(1.5f));
}

TEST_CASE("ModMatrixExecutor routes LFO to OperatorWavetableSyncAmount with targetIndex", "[modulation][warp]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo4, ModDestination::OperatorWavetableSyncAmount, 1, 0.5f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[3] = 0.8f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorWavetableSyncAmountOffset[1] == Catch::Approx(0.4f));
    REQUIRE(out.operatorWavetableSyncAmountOffset[0] == 0.0f);
}

TEST_CASE("ModMatrixExecutor routes LFO to OperatorWavetableFormant with targetIndex", "[modulation][warp]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Env2, ModDestination::OperatorWavetableFormant, 4, 0.6f, ModScope::Voice});

    ModSourceValues sources;
    sources.envelopes[1] = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorWavetableFormantOffset[4] == Catch::Approx(0.6f));
    REQUIRE(out.operatorWavetableFormantOffset[0] == 0.0f);
}

TEST_CASE("ModMatrixExecutor resolves macro sources by index", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Macro3, ModDestination::Pan, 0, 1.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.macros[2] = 0.75f; // Macro3 == index 2.

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.panOffset == Catch::Approx(0.75f));
}

TEST_CASE("ModMatrixExecutor applyMasterBus routes Macro2 to master reverb at Global scope", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Macro2, ModDestination::MasterReverbMix, 2, 0.35f, ModScope::Global});
    routes.push_back(ModRoute{ModSource::Macro2, ModDestination::MasterReverbSize, 2, 0.5f, ModScope::Global});
    routes.push_back(ModRoute{ModSource::Macro1, ModDestination::FilterCutoff, 0, 12.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.macros[1] = 1.0f;

    const auto out = ModMatrixExecutor::applyMasterBus(routes, sources);
    REQUIRE(out.mixOffset[2] == Catch::Approx(0.35f));
    REQUIRE(out.reverbSizeOffset[2] == Catch::Approx(0.5f));
    REQUIRE(out.mixOffset[0] == 0.0f);
}

TEST_CASE("ModMatrixExecutor applyMasterBus ignores Voice-scoped master routes", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Macro2, ModDestination::MasterReverbMix, 2, 0.35f, ModScope::Voice});

    ModSourceValues sources;
    sources.macros[1] = 1.0f;

    const auto out = ModMatrixExecutor::applyMasterBus(routes, sources);
    REQUIRE(out.mixOffset[2] == 0.0f);
}

TEST_CASE("ModMatrixExecutor routes Sidechain to FilterCutoff", "[modulation][sidechain]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Sidechain, ModDestination::FilterCutoff, 0, 24.0f, ModScope::Global});

    ModSourceValues sources;
    sources.sidechain = 0.5f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == Catch::Approx(12.0f));
}

TEST_CASE("ModMatrixExecutor routes LFO to Blades filter destinations", "[modulation][blades]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Lfo1, ModDestination::FilterModeMorph, 0, 0.5f, ModScope::Layer});
    routes.push_back(ModRoute{ModSource::Lfo2, ModDestination::FilterRouting, 0, 0.4f, ModScope::Global});
    routes.push_back(ModRoute{ModSource::Env1, ModDestination::FilterDrive, 0, 0.3f, ModScope::Voice});

    ModSourceValues sources;
    sources.layerLfos[0] = 1.0f;
    sources.layerLfos[1] = 0.5f;
    sources.envelopes[0] = 0.75f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterModeMorphOffset == Catch::Approx(0.5f));
    REQUIRE(out.filterRoutingOffset == Catch::Approx(0.2f));
    REQUIRE(out.filterDriveOffset == Catch::Approx(0.225f));
}

TEST_CASE("ModMatrixExecutor routes Sidechain to VocoderMix on master slot", "[modulation][sidechain][vocoder]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Sidechain, ModDestination::VocoderMix, 1, 0.8f, ModScope::Global});

    ModSourceValues sources;
    sources.sidechain = 0.75f;

    const auto out = ModMatrixExecutor::applyMasterBus(routes, sources);
    REQUIRE(out.masterVocoderMixOffset[1] == Catch::Approx(0.6f));
}

TEST_CASE("ModMatrixExecutor routes Sidechain to VocoderFormant on master slot", "[modulation][sidechain][vocoder]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Sidechain, ModDestination::VocoderFormant, 0, 1.0f, ModScope::Global});

    ModSourceValues sources;
    sources.sidechain = 0.4f;

    const auto out = ModMatrixExecutor::applyMasterBus(routes, sources);
    REQUIRE(out.masterVocoderFormantOffset[0] == Catch::Approx(0.4f));
}

TEST_CASE("shapeModSource exponential curve reduces mid-range modulation", "[modulation][curve]")
{
    REQUIRE(shapeModSource(0.5f, ModCurve::Linear) == Catch::Approx(0.5f));
    REQUIRE(shapeModSource(0.5f, ModCurve::Exponential) == Catch::Approx(0.25f));
    REQUIRE(shapeModSource(0.5f, ModCurve::Logarithmic) == Catch::Approx(std::sqrt(0.5f)));
    REQUIRE(shapeModSource(-0.5f, ModCurve::Exponential) == Catch::Approx(-0.25f));
}

TEST_CASE("ModMatrixExecutor applies route curve shaping before amount scaling", "[modulation][curve]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    ModRoute route{ModSource::Velocity, ModDestination::FilterCutoff, 0, 12.0f, ModScope::Voice};
    route.curve = ModCurve::Exponential;
    routes.push_back(route);

    ModSourceValues sources;
    sources.velocity = 0.5f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.filterCutoffSemitones == Catch::Approx(3.0f));
}

TEST_CASE("ModMatrixExecutor routes LFO to OperatorFmModulatorIndex with targetIndex", "[modulation][fm]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(
        ModRoute{ModSource::Lfo1, ModDestination::OperatorFmModulatorIndex, 2, 0.5f, ModScope::Voice});

    ModSourceValues sources;
    sources.voiceLfos[0] = 1.0f;

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.operatorFmModulatorIndexOffset[2] == Catch::Approx(0.5f));
}

TEST_CASE("ModMatrixExecutor computeLayerUnisonMod applies layer-wide unison routes", "[modulation][unison]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Macro1, ModDestination::UnisonVoices, 0, 2.0f, ModScope::Layer});
    routes.push_back(ModRoute{ModSource::Macro2, ModDestination::UnisonDetune, 0, 10.0f, ModScope::Global});
    routes.push_back(ModRoute{ModSource::Lfo3, ModDestination::UnisonSpread, 0, 0.25f, ModScope::Voice});

    ModSourceValues sources;
    sources.macros[0] = 1.0f;
    sources.macros[1] = 0.5f;

    const auto out = ModMatrixExecutor::computeLayerUnisonMod(routes, sources);
    REQUIRE(out.unisonVoicesOffset == Catch::Approx(2.0f));
    REQUIRE(out.unisonDetuneOffset == Catch::Approx(5.0f));
    REQUIRE(out.unisonSpreadOffset == Catch::Approx(0.0f));
}
