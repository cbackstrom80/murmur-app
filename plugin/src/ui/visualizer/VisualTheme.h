#pragma once

#include <cstdint>

#include "../theme/ObsidianPalette.h"
#include "VisualPreviewCache.h"

namespace pw8::plugin::ui::preview
{

[[nodiscard]] inline uint64_t visualCacheThemeKey() noexcept
{
    uint64_t h = 0x7E3E00001ULL;
    h = hashCombine(h, static_cast<uint64_t>(palette::kAccent.getARGB()));
    h = hashCombine(h, static_cast<uint64_t>(palette::kAccentWarm.getARGB()));
    h = hashCombine(h, static_cast<uint64_t>(palette::kAccentDim.getARGB()));
    h = hashCombine(h, static_cast<uint64_t>(palette::kBackgroundBottom.getARGB()));
    h = hashCombine(h, static_cast<uint64_t>(palette::kBorder.getARGB()));
    return h;
}

[[nodiscard]] inline uint64_t withVisualThemeKey(uint64_t paramKey) noexcept
{
    return hashCombine(paramKey, visualCacheThemeKey());
}

} // namespace pw8::plugin::ui::preview
