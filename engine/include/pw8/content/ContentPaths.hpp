#pragma once

#include <optional>
#include <string>
#include <vector>

// Resolves repo-relative content references (wavetableId paths like
// "content/wavetables/foo.json") against known install and dev search roots.
// Control-path only; safe to call from Engine::loadPatch() on the message thread.
namespace pw8::content
{
    /// Clears explicit roots (unit tests only).
    void resetSearchRootsForTests() noexcept;

    /// Prepends an additional root (absolute directory, e.g. repo root or
    /// Application Support/Patchwork Eight). Later calls take precedence.
    void addSearchRoot(const std::string& rootPath) noexcept;

    /// Returns an existing filesystem path for `wavetableId`, or nullopt if none found.
    /// Tries the id as-is (absolute or cwd-relative) before searching roots.
    [[nodiscard]] std::optional<std::string> resolveWavetablePath(const std::string& wavetableId) noexcept;

    /// Preset library directories to scan (factory, showcase, dev tree).
    [[nodiscard]] std::vector<std::string> presetSearchRoots() noexcept;

    /// Wavetable library directories (content/wavetables under each root).
    [[nodiscard]] std::vector<std::string> wavetableSearchRoots() noexcept;

} // namespace pw8::content
