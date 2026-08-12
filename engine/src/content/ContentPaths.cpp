#include "pw8/content/ContentPaths.hpp"

#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <vector>

namespace fs = std::filesystem;

namespace pw8::content
{
    namespace
    {
        std::mutex rootsMutex;
        std::vector<fs::path> extraRoots;
        bool defaultsRegistered = false;

        [[nodiscard]] bool fileExists(const fs::path& p)
        {
            std::error_code ec;
            return fs::is_regular_file(p, ec);
        }

        void registerDefaultRootsLocked()
        {
            if (defaultsRegistered)
                return;
            defaultsRegistered = true;

            if (const char* envRoot = std::getenv("PW8_CONTENT_ROOT"))
            {
                if (envRoot[0] != '\0')
                    extraRoots.push_back(fs::path(envRoot));
            }

#if defined(__APPLE__)
            extraRoots.push_back(fs::path("/Library/Application Support/Patchwork Eight"));
            if (const char* home = std::getenv("HOME"))
                extraRoots.push_back(fs::path(home) / "Library/Application Support/Patchwork Eight");
#endif

            // Dev convenience: if cwd or its parents contain content/wavetables/, use that tree.
            std::error_code ec;
            auto cwd = fs::current_path(ec);
            if (!ec)
            {
                for (int depth = 0; depth < 6 && !cwd.empty(); ++depth)
                {
                    if (fileExists(cwd / "content/wavetables"))
                    {
                        extraRoots.push_back(cwd);
                        break;
                    }
                    const auto parent = cwd.parent_path();
                    if (parent == cwd)
                        break;
                    cwd = parent;
                }
            }
        }

        [[nodiscard]] std::vector<fs::path> allRoots()
        {
            std::lock_guard lock(rootsMutex);
            registerDefaultRootsLocked();
            return extraRoots;
        }

        [[nodiscard]] std::optional<std::string> tryPath(const fs::path& candidate)
        {
            if (fileExists(candidate))
                return candidate.string();
            return std::nullopt;
        }

        [[nodiscard]] fs::path basenameFromId(const std::string& wavetableId)
        {
            const fs::path p(wavetableId);
            return p.filename();
        }
    } // namespace

    void resetSearchRootsForTests() noexcept
    {
        std::lock_guard lock(rootsMutex);
        extraRoots.clear();
        defaultsRegistered = false;
    }

    void addSearchRoot(const std::string& rootPath) noexcept
    {
        if (rootPath.empty())
            return;
        std::lock_guard lock(rootsMutex);
        registerDefaultRootsLocked();
        extraRoots.insert(extraRoots.begin(), fs::path(rootPath));
    }

    std::optional<std::string> resolveWavetablePath(const std::string& wavetableId) noexcept
    {
        if (wavetableId.empty())
            return std::nullopt;

        const fs::path raw(wavetableId);
        if (raw.is_absolute())
        {
            if (auto hit = tryPath(raw))
                return hit;
        }
        else if (auto hit = tryPath(raw))
            return hit;

        const auto base = basenameFromId(wavetableId);
        for (const auto& root : allRoots())
        {
            if (auto hit = tryPath(root / wavetableId))
                return hit;
            if (auto hit = tryPath(root / "content/wavetables" / base))
                return hit;
            if (auto hit = tryPath(root / "Wavetables" / base))
                return hit;
        }

        return std::nullopt;
    }

    std::vector<std::string> presetSearchRoots() noexcept
    {
        std::vector<std::string> out;
        for (const auto& root : allRoots())
        {
            out.push_back((root / "content/presets").string());
            out.push_back((root / "Presets").string());
            out.push_back((root / "Presets/factory").string());
            out.push_back((root / "Presets/showcase").string());
        }
        return out;
    }

    std::vector<std::string> wavetableSearchRoots() noexcept
    {
        std::vector<std::string> out;
        for (const auto& root : allRoots())
        {
            out.push_back((root / "content/wavetables").string());
            out.push_back((root / "Wavetables").string());
        }
        return out;
    }

} // namespace pw8::content
