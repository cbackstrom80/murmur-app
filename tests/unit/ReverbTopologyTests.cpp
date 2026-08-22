#include <catch2/catch_test_macros.hpp>

#include "pw8/effects/ReverbTopology.hpp"

using namespace pw8::effects;

// Real, direct tests of getReverbTopology() -- the table-level proof that
// Plate/Hall/Room/Spring's new topologies are genuinely distinct (not
// copy-paste placeholders), that Default's table is untouched from what
// Reverb.hpp used to hardcode directly (the backward-compatibility claim,
// checked here rather than only inferred from the golden-hash test passing),
// and that every real value stays inside the engine's actual buffer
// ceilings.
namespace
{
    bool baseDelaysEqual(const ReverbTopology& a, const ReverbTopology& b)
    {
        for (std::size_t i = 0; i < kNumReverbLines; ++i)
            if (a.baseDelayMs[i] != b.baseDelayMs[i])
                return false;
        return true;
    }
} // namespace

TEST_CASE("Default reverb topology matches the engine's original hardcoded table", "[reverb][topology]")
{
    const auto& topo = getReverbTopology(ReverbCharacter::Default);
    REQUIRE(topo.activeLines == kNumReverbLines);
    const std::array<float, kNumReverbLines> expected = {23.1f, 29.7f, 31.3f, 37.1f, 41.3f, 43.7f, 47.9f, 53.3f};
    for (std::size_t i = 0; i < kNumReverbLines; ++i)
        REQUIRE(topo.baseDelayMs[i] == expected[i]);
    const std::array<float, kNumReverbDiffuserStages> expectedDiffuser = {4.7f, 6.1f, 7.9f, 9.7f};
    for (std::size_t i = 0; i < kNumReverbDiffuserStages; ++i)
        REQUIRE(topo.diffuserStageMs[i] == expectedDiffuser[i]);
}

TEST_CASE("Shimmer deliberately reuses Default's topology (documented, not an oversight)", "[reverb][topology]")
{
    const auto& def = getReverbTopology(ReverbCharacter::Default);
    const auto& shimmer = getReverbTopology(ReverbCharacter::Shimmer);
    REQUIRE(baseDelaysEqual(def, shimmer));
    REQUIRE(def.activeLines == shimmer.activeLines);
}

TEST_CASE("Plate/Hall/Room/Spring have genuinely distinct base delay-length tables", "[reverb][topology]")
{
    const auto& def = getReverbTopology(ReverbCharacter::Default);
    const auto& plate = getReverbTopology(ReverbCharacter::Plate);
    const auto& hall = getReverbTopology(ReverbCharacter::Hall);
    const auto& room = getReverbTopology(ReverbCharacter::Room);
    const auto& spring = getReverbTopology(ReverbCharacter::Spring);

    const ReverbTopology* topologies[] = {&def, &plate, &hall, &room, &spring};
    for (std::size_t a = 0; a < 5; ++a)
        for (std::size_t b = a + 1; b < 5; ++b)
            REQUIRE_FALSE(baseDelaysEqual(*topologies[a], *topologies[b]));
}

TEST_CASE("Spring uses fewer active lines than the other characters -- a real structural difference",
          "[reverb][topology]")
{
    REQUIRE(getReverbTopology(ReverbCharacter::Spring).activeLines < kNumReverbLines);
    REQUIRE(getReverbTopology(ReverbCharacter::Spring).activeLines > 0);
    REQUIRE(getReverbTopology(ReverbCharacter::Default).activeLines == kNumReverbLines);
    REQUIRE(getReverbTopology(ReverbCharacter::Plate).activeLines == kNumReverbLines);
    REQUIRE(getReverbTopology(ReverbCharacter::Hall).activeLines == kNumReverbLines);
    REQUIRE(getReverbTopology(ReverbCharacter::Room).activeLines == kNumReverbLines);
}

TEST_CASE("Every real topology's values stay inside the engine's actual delay-line/diffuser buffer ceilings",
          "[reverb][topology]")
{
    // kMaxReverbLineSeconds/kMaxReverbDiffuserStageSeconds are the real
    // buffer sizes DelayLine::prepare() allocates in Reverb.hpp -- a table
    // entry at or above these (even before reverbSizeParam scaling, which
    // can push it further) would be a real authoring mistake, not just a
    // style nit.
    constexpr float kMaxLineMs = kMaxReverbLineSeconds * 1000.0f;
    constexpr float kMaxDiffuserMs = kMaxReverbDiffuserStageSeconds * 1000.0f;

    for (const auto character : {ReverbCharacter::Default, ReverbCharacter::Plate, ReverbCharacter::Hall,
                                  ReverbCharacter::Room, ReverbCharacter::Spring, ReverbCharacter::Shimmer})
    {
        const auto& topo = getReverbTopology(character);
        for (std::size_t i = 0; i < topo.activeLines; ++i)
        {
            REQUIRE(topo.baseDelayMs[i] > 0.0f);
            REQUIRE(topo.baseDelayMs[i] < kMaxLineMs);
        }
        for (const float ms : topo.diffuserStageMs)
        {
            REQUIRE(ms > 0.0f);
            REQUIRE(ms < kMaxDiffuserMs);
        }
    }
}
