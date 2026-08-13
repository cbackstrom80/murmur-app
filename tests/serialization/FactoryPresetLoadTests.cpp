#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/content/ContentPaths.hpp"
#include "pw8/core/Version.hpp"
#include "pw8/patch/PatchSerializer.hpp"

using namespace pw8;
namespace fs = std::filesystem;

namespace
{
    [[nodiscard]] fs::path repoRoot() noexcept
    {
#ifdef PW8_REPO_ROOT
        return fs::path(PW8_REPO_ROOT);
#else
        return fs::current_path();
#endif
    }

    [[nodiscard]] patch::Patch loadPresetFile(const fs::path& absPath)
    {
        std::ifstream in(absPath);
        REQUIRE(in.good());
        std::ostringstream ss;
        ss << in.rdbuf();
        const auto loaded = patch::loadPatchFromJson(ss.str());
        REQUIRE(loaded.ok);
        return loaded.patch;
    }

    [[nodiscard]] std::vector<fs::path> week3FactoryPresets()
    {
        const fs::path factoryRoot = repoRoot() / "content/presets/factory";
        std::vector<fs::path> paths;

        for (const char* subdir : {"Warp", "Templates", "Interstellar"})
        {
            const fs::path dir = factoryRoot / subdir;
            if (!fs::is_directory(dir))
                continue;
            for (const auto& entry : fs::directory_iterator(dir))
            {
                if (entry.path().extension() == ".pw8")
                    paths.push_back(entry.path());
            }
        }

        return paths;
    }
} // namespace

TEST_CASE("Week 3 factory presets load under schema v3", "[patch][serialization][factory]")
{
    content::resetSearchRootsForTests();
    content::addSearchRoot(repoRoot().string());

    const auto presets = week3FactoryPresets();
    REQUIRE_FALSE(presets.empty());

    for (const auto& path : presets)
    {
        INFO("preset: " << path.filename().string());
        const auto patch = loadPresetFile(path);
        REQUIRE(patch.schemaVersion == core::kPatchSchemaVersion);
        REQUIRE(patch.layerA.operators.size() == core::kNodesPerLayer);
    }
}

TEST_CASE("Interstellar factory presets load under schema v3", "[patch][serialization][factory]")
{
    content::resetSearchRootsForTests();
    content::addSearchRoot(repoRoot().string());

    const fs::path dir = repoRoot() / "content/presets/factory/Interstellar";
    if (!fs::is_directory(dir))
        SKIP("Missing content/presets/factory/Interstellar/");

    std::size_t count = 0;
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.path().extension() != ".pw8")
            continue;
        INFO("preset: " << entry.path().filename().string());
        const auto patch = loadPresetFile(entry.path());
        REQUIRE(patch.schemaVersion == core::kPatchSchemaVersion);
        REQUIRE(patch.layerA.operators.size() == core::kNodesPerLayer);
        REQUIRE(patch.metadata.category == "interstellar");
        ++count;
    }
    REQUIRE(count == 100);
}

TEST_CASE("warp-bend-demo preset has audible bend on OP0", "[patch][serialization][factory]")
{
    const auto path = repoRoot() / "content/presets/factory/Warp/warp-bend-demo.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing content/presets/factory/Warp/warp-bend-demo.pw8");

    const auto patch = loadPresetFile(path);
    REQUIRE(patch.layerA.operators[0].engine == algorithm::EngineType::Wavetable);
    REQUIRE(std::abs(patch.layerA.operators[0].wtBend) > 0.1f);
}

TEST_CASE("feedback-bell factory preset matches template topology", "[patch][serialization][factory]")
{
    const auto path = repoRoot() / "content/presets/factory/Templates/feedback-bell.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing content/presets/factory/Templates/feedback-bell.pw8");

    const auto patch = loadPresetFile(path);
    REQUIRE(patch.layerA.algorithm.edges.size() == 3);

    bool hasFeedback = false;
    bool hasPmToCarrier = false;
    for (const auto& edge : patch.layerA.algorithm.edges)
    {
        if (edge.type == algorithm::EdgeType::Feedback && edge.source.get() == 1 && edge.destination.get() == 1)
            hasFeedback = true;
        if (edge.type == algorithm::EdgeType::PhaseMod && edge.source.get() == 1 && edge.destination.get() == 0)
            hasPmToCarrier = true;
    }
    REQUIRE(hasFeedback);
    REQUIRE(hasPmToCarrier);
}
