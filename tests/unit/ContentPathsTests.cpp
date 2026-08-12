#include <catch2/catch_test_macros.hpp>

#include "pw8/content/ContentPaths.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST_CASE("resolveWavetablePath finds tables under a registered root", "[content][paths]")
{
    pw8::content::resetSearchRootsForTests();

    const auto tempRoot = fs::temp_directory_path() / "pw8_content_paths_test";
    fs::remove_all(tempRoot);
    fs::create_directories(tempRoot / "Wavetables");

    {
        std::ofstream out(tempRoot / "Wavetables" / "probe-table.json");
        out << R"({"schemaVersion":2,"numFrames":1,"frameLength":4,"numMipLevels":1,"mips":[{"maxHarmonic":1,"frames":[[0,0,0,0]]}]})";
    }

    pw8::content::addSearchRoot(tempRoot.string());

    const auto resolved = pw8::content::resolveWavetablePath("content/wavetables/probe-table.json");
    REQUIRE(resolved.has_value());
    REQUIRE(resolved->find("probe-table.json") != std::string::npos);

    fs::remove_all(tempRoot);
    pw8::content::resetSearchRootsForTests();
}

TEST_CASE("addSearchRootsFromAncestorWalk registers repo-style content tree", "[content][paths]")
{
    pw8::content::resetSearchRootsForTests();

    const auto tempRoot = fs::temp_directory_path() / "pw8_content_ancestor_test";
    fs::remove_all(tempRoot);
    fs::create_directories(tempRoot / "content/wavetables");
    fs::create_directories(tempRoot / "build/app/Contents/MacOS");

    pw8::content::addSearchRootsFromAncestorWalk((tempRoot / "build/app/Contents/MacOS/fake-binary").string());

    const auto resolved = pw8::content::resolveWavetablePath("content/wavetables/anything.json");
    REQUIRE_FALSE(pw8::content::wavetableSearchRoots().empty());

    fs::remove_all(tempRoot);
    pw8::content::resetSearchRootsForTests();
}
