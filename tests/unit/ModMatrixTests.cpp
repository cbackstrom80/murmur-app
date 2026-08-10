#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/core/Types.hpp"
#include "pw8/modulation/ModMatrixExecutor.hpp"

using namespace pw8::modulation;

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

TEST_CASE("ModMatrixExecutor resolves macro sources by index", "[modulation]")
{
    pw8::core::FixedVector<ModRoute, pw8::core::kMaxModRoutes> routes;
    routes.push_back(ModRoute{ModSource::Macro3, ModDestination::Pan, 0, 1.0f, ModScope::Voice});

    ModSourceValues sources;
    sources.macros[2] = 0.75f; // Macro3 == index 2.

    const auto out = ModMatrixExecutor::apply(routes, sources);
    REQUIRE(out.panOffset == Catch::Approx(0.75f));
}
