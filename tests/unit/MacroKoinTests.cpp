#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "pw8/modulation/ModMatrixTypes.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/patch/PatchModDefaults.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/render/Engine.hpp"

using namespace pw8;

namespace
{
    constexpr double kSampleRate = 48000.0;

    [[nodiscard]] patch::Patch loadPresetFromRepo(const char* relativePath)
    {
#ifdef PW8_REPO_ROOT
        const std::filesystem::path path = std::filesystem::path(PW8_REPO_ROOT) / relativePath;
#else
        const std::filesystem::path path = relativePath;
#endif
        std::ifstream in(path);
        REQUIRE(in.good());
        std::ostringstream ss;
        ss << in.rdbuf();
        const auto loaded = patch::loadPatchFromJson(ss.str());
        REQUIRE(loaded.ok);
        return loaded.patch;
    }

    [[nodiscard]] float renderRms(render::Engine& engine, int numSamples)
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

    [[nodiscard]] bool patchHasMacroRoute(const patch::Patch& patch, std::size_t macroIndex)
    {
        const auto source = static_cast<modulation::ModSource>(static_cast<int>(modulation::ModSource::Macro1) +
                                                               static_cast<int>(macroIndex));
        for (const auto& route : patch.layerA.modRoutes)
        {
            if (route.isActive() && route.source == source)
                return true;
        }
        return false;
    }
} // namespace

TEST_CASE("ensureMinimumMacroKoinRoutes adds Macro1 filter routes to Init", "[patch][macro][koin]")
{
    auto patch = patch::Patch::makeInit();
    REQUIRE_FALSE(patch::layerHasActiveMacroRoute(patch.layerA));

    patch::ensureMinimumMacroKoinRoutes(patch.layerA);
    REQUIRE(patch::layerHasActiveMacroRoute(patch.layerA));
    REQUIRE(patchHasMacroRoute(patch, 0));
}

TEST_CASE("CATHEDRAL NEBULA macro1 modulates a held voice", "[patch][macro][factory][koin]")
{
    auto patch = loadPresetFromRepo("content/presets/factory/Interstellar/001-cathedral-nebula.pw8");
    REQUIRE(patchHasMacroRoute(patch, 0));
    REQUIRE_FALSE(patch.uiFocus.knobs.empty());

    render::Engine engine;
    engine.prepare(kSampleRate);
    REQUIRE(engine.loadPatch(patch));

    engine.noteOn(60, 0, 100);

    std::vector<float> settleL(4000), settleR(4000);
    core::StereoBlockView settleView(settleL.data(), settleR.data(), settleL.size());
    engine.process(settleView);

    const float rmsOpen = renderRms(engine, 2000);
    REQUIRE(rmsOpen > 0.01f);

    engine.setMacroValue(0, 1.0f);
    const float rmsMacro = renderRms(engine, 2000);
    REQUIRE(std::abs(rmsMacro - rmsOpen) > 0.002f);
}

TEST_CASE("Factory presets expose at least one uiFocus macro KOIN", "[patch][serialization][factory][koin]")
{
#ifdef PW8_REPO_ROOT
    const std::filesystem::path factoryRoot = std::filesystem::path(PW8_REPO_ROOT) / "content/presets/factory";
#else
    const std::filesystem::path factoryRoot = "content/presets/factory";
#endif

    std::size_t checked = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(factoryRoot))
    {
        if (entry.path().extension() != ".pw8")
            continue;

        std::ifstream in(entry.path());
        REQUIRE(in.good());
        std::ostringstream ss;
        ss << in.rdbuf();
        const auto loaded = patch::loadPatchFromJson(ss.str());
        REQUIRE(loaded.ok);

        const auto& focus = loaded.patch.uiFocus;
        REQUIRE_FALSE(focus.knobs.empty());
        bool hasMacroKoin = false;
        for (const auto& knob : focus.knobs)
        {
            if (knob.kind != patch::UiFocusKnobKind::Macro)
                continue;
            hasMacroKoin = true;
            REQUIRE(patchHasMacroRoute(loaded.patch, knob.macroIndex));
        }
        REQUIRE(hasMacroKoin);
        ++checked;
    }

    REQUIRE(checked >= 900);
}
