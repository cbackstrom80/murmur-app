#include <catch2/catch_test_macros.hpp>

#include "pw8/oscillator/WavetableTableLoader.hpp"

using namespace pw8::oscillator;

namespace
{
    // 1 frame x 4 samples, 2 mip levels -- minimal but structurally real.
    constexpr const char* kValidJson = R"({
        "schemaVersion": 2,
        "numFrames": 1,
        "samplesPerFrame": 4,
        "mips": [
            {"maxHarmonic": 1, "frames": [[0.0, 1.0, 0.0, -1.0]]},
            {"maxHarmonic": 0, "frames": [[0.1, 0.1, 0.1, 0.1]]}
        ]
    })";
}

TEST_CASE("loadWavetableFromJson parses a valid multi-mip table", "[wavetable][serialization]")
{
    const auto result = loadWavetableFromJson(kValidJson);
    REQUIRE(result.ok);
    REQUIRE(result.table.numFrames == 1);
    REQUIRE(result.table.samplesPerFrame == 4);
    REQUIRE(result.table.mips.size() == 2);
    REQUIRE(result.table.mips[0].maxHarmonic == 1);
    REQUIRE(result.table.mips[0].samples.size() == 4);
    REQUIRE(result.table.mips[0].samples[1] == 1.0f);
    REQUIRE(result.table.isValid());
}

TEST_CASE("loadWavetableFromJson rejects malformed JSON", "[wavetable][serialization][robustness]")
{
    const auto result = loadWavetableFromJson("{not valid json");
    REQUIRE_FALSE(result.ok);
    REQUIRE_FALSE(result.error.empty());
}

TEST_CASE("loadWavetableFromJson rejects a mip whose sample count doesn't match numFrames*samplesPerFrame",
          "[wavetable][serialization][robustness]")
{
    constexpr const char* badJson = R"({
        "numFrames": 1,
        "samplesPerFrame": 4,
        "mips": [
            {"maxHarmonic": 1, "frames": [[0.0, 1.0]]}
        ]
    })";
    const auto result = loadWavetableFromJson(badJson);
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("loadWavetableFromJson rejects out-of-range numFrames/samplesPerFrame", "[wavetable][serialization][robustness]")
{
    constexpr const char* badJson = R"({"numFrames": 0, "samplesPerFrame": 4, "mips": []})";
    const auto result = loadWavetableFromJson(badJson);
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("loadWavetableFromFile reports a clear error for a missing file", "[wavetable][serialization][robustness]")
{
    const auto result = loadWavetableFromFile("/nonexistent/path/does-not-exist.json");
    REQUIRE_FALSE(result.ok);
    REQUIRE_FALSE(result.error.empty());
}
