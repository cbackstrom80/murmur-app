#pragma once

#include <array>

#include <juce_core/juce_core.h>

namespace pw8::fathom
{
    /// Real, honest keyword-based grouping of the 38 real bundled impulse
    /// responses (Voxengo Impulse Modeler, Aleksey Vaneev -- royalty-free
    /// incl. commercial use, see resources/impulse-responses/
    /// VOXENGO_IR_LICENSE.txt, which must ship alongside these files per
    /// its own redistribution terms). "Cabinet" is kept as its own honestly-
    /// labeled category rather than hidden or dropped -- these 4 really are
    /// short guitar-cabinet impulse responses, not room/hall reverbs, and a
    /// reverb product shouldn't blur that distinction.
    enum class IrCategory
    {
        Room,
        Hall,
        Cabinet,
        Special,
    };

    struct IrEntry
    {
        const char* displayName;
        const char* fileName; // relative to the bundle's Resources/impulse-responses/
        IrCategory category;
    };

    inline constexpr std::size_t kNumBundledIrs = 38;

    extern const std::array<IrEntry, kNumBundledIrs> kBundledIrs;

    [[nodiscard]] const char* irCategoryLabel(IrCategory category) noexcept;

    /// Real bundle Resources/impulse-responses/ directory -- same
    /// "relative to the running plugin binary's bundle" resolution
    /// QuasarPreset.cpp's factoryPresetsDirectory() already uses.
    [[nodiscard]] juce::File impulseResponsesDirectory();

} // namespace pw8::fathom
