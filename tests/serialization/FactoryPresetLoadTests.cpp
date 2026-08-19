#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/content/ContentPaths.hpp"
#include "pw8/core/Version.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"
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

TEST_CASE("vocal-ext-vocoder sidechain showcase preset loads", "[patch][serialization][factory][sidechain]")
{
    const auto path = repoRoot() / "content/presets/factory/Sidechain/01-vocal-ext-vocoder.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing content/presets/factory/Sidechain/01-vocal-ext-vocoder.pw8");

    const auto patch = loadPresetFile(path);
    REQUIRE(patch.layerA.operators[0].engine == algorithm::EngineType::External);
    REQUIRE(patch.layerA.insertEffects[0].type == effects::EffectType::Vocoder);
    REQUIRE(patch.layerA.insertEffects[0].vocoderBandCount == 12);

    bool hasSidechainVocoderFormant = false;
    for (const auto& route : patch.layerA.modRoutes)
    {
        if (route.source == modulation::ModSource::Sidechain &&
            route.destination == modulation::ModDestination::VocoderFormant && route.targetIndex == 0)
            hasSidechainVocoderFormant = true;
    }
    REQUIRE(hasSidechainVocoderFormant);
}

TEST_CASE("Imogen Hide & Seek vocoder preset loads with 16 bands", "[patch][serialization][factory][sidechain]")
{
    const auto path = repoRoot() / "content/presets/factory/Sidechain/02-imogen-hide-seek-vocoder.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing Sidechain/02-imogen-hide-seek-vocoder.pw8");

    const auto patch = loadPresetFile(path);
    REQUIRE(patch.layerA.insertEffects[0].type == effects::EffectType::Vocoder);
    REQUIRE(patch.layerA.insertEffects[0].vocoderBandCount == 16);
    REQUIRE(patch.layerA.insertEffects[0].mix > 0.95f);
}

TEST_CASE("Def Leppard Love Bites vocoder preset loads rock stack", "[patch][serialization][factory][sidechain]")
{
    const auto path = repoRoot() / "content/presets/factory/Sidechain/05-def-leppard-love-bites-vocoder.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing Sidechain/05-def-leppard-love-bites-vocoder.pw8");

    const auto patch = loadPresetFile(path);
    REQUIRE(patch.layerA.insertEffects[0].type == effects::EffectType::Vocoder);
    REQUIRE(patch.layerA.insertEffects[0].vocoderBandCount == 8);
    REQUIRE(patch.layerA.insertEffects[0].vocoderFormant > 0.6f);
    REQUIRE(patch.layerA.insertEffects[0].mix > 0.8f);
    REQUIRE(static_cast<int>(patch.layerA.operators[0].classicWaveform) == 2);
}

TEST_CASE("Interstellar Spatial preset embeds inline Quasar on master M3", "[patch][serialization][factory][quasar]")
{
    const auto path = repoRoot() / "content/presets/factory/Interstellar/Spatial/001-nebula-drift.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing Interstellar/Spatial/001-nebula-drift.pw8");

    std::ifstream in(path);
    REQUIRE(in.good());
    std::ostringstream ss;
    ss << in.rdbuf();
    const auto loaded = patch::loadPatchFromJson(ss.str());
    INFO("load error: " << loaded.error);
    REQUIRE(loaded.ok);
    const auto& patch = loaded.patch;
    REQUIRE(patch.masterEffects[2].type == effects::EffectType::BinauralSpace);
    REQUIRE(patch.masterEffects[2].mix > 0.5f);
    REQUIRE(patch.masterEffects[2].qsr1Level > 0.0f);
    REQUIRE(patch.metadata.description.find("inline QUASAR") != std::string::npos);
}

TEST_CASE("Interstellar legacy type-11 Quasar master slot loads as BinauralSpace", "[patch][serialization][factory]")
{
    const auto path = repoRoot() / "content/presets/factory/Interstellar/001-cathedral-nebula.pw8";
    if (!fs::is_regular_file(path))
        SKIP("Missing Interstellar/001-cathedral-nebula.pw8");

    const auto patch = loadPresetFile(path);
    REQUIRE(patch.masterEffects[2].type == effects::EffectType::BinauralSpace);
    REQUIRE(patch.masterEffects[2].mix == Catch::Approx(0.72f));
    REQUIRE(patch.masterEffects[2].qsr1Level == Catch::Approx(0.65f));
}
