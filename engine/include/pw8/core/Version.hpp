#pragma once

#include <cstdint>
#include <string_view>

namespace pw8::core
{
    /// Semantic version of the pw8_core engine itself (DSP behaviour, not the patch schema).
    struct EngineVersion
    {
        static constexpr int major = 0;
        static constexpr int minor = 1;
        static constexpr int patch = 0;

        [[nodiscard]] static constexpr std::string_view string() noexcept
        {
            return "0.1.0";
        }
    };

    /// Schema version of the native `.pw8` patch file format.
    /// Bump this whenever the on-disk JSON shape changes in a way that requires migration.
    inline constexpr int kPatchSchemaVersion = 1;

    /// Schema version of `AlgorithmGraphDefinition` serialization.
    inline constexpr int kAlgorithmSchemaVersion = 1;

} // namespace pw8::core
