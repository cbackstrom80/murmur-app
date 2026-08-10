#pragma once

#include <string>
#include <string_view>

#include "pw8/oscillator/WavetableTable.hpp"

// JSON (de)serialization for WavetableTable -- the only place nlohmann::json is
// allowed to touch wavetable data, kept out of WavetableTable.hpp itself for the
// same reason PatchSerializer is kept out of Patch.hpp (see docs/PATCH_FORMAT.md).
// Control-path only: load from a file during Engine::loadPatch() (background/UI
// thread), never from the audio thread.

namespace pw8::oscillator
{
    struct WavetableLoadResult
    {
        bool ok = false;
        WavetableTable table{};
        std::string error;
    };

    [[nodiscard]] WavetableLoadResult loadWavetableFromJson(std::string_view jsonText) noexcept;
    [[nodiscard]] WavetableLoadResult loadWavetableFromFile(const std::string& path) noexcept;

} // namespace pw8::oscillator
