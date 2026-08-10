#pragma once

#include <string>
#include <string_view>

#include "pw8/patch/Patch.hpp"

// The ONLY boundary in pw8_core where JSON parsing happens. Loading/saving a .pw8
// file is control-path work (UI/background thread) -- never call these from the
// audio thread. Treats input as untrusted: malformed or hostile JSON returns a
// failure result rather than throwing into caller code or allocating unbounded memory.

namespace pw8::patch
{
    struct PatchLoadResult
    {
        bool ok = false;
        Patch patch{};
        std::string error;
        /// Schema version the document was originally written at, before migration.
        int originalSchemaVersion = core::kPatchSchemaVersion;
    };

    [[nodiscard]] PatchLoadResult loadPatchFromJson(std::string_view jsonText) noexcept;

    /// Serializes to a pretty-printed JSON string. `indent < 0` produces compact JSON.
    [[nodiscard]] std::string savePatchToJson(const Patch& patch, int indent = 2) noexcept;

} // namespace pw8::patch
