#pragma once

#include "components/PatchBrowserBar.h"
#include "components/PresetBrowserOverlay.h"
#include "content/FavoritesStore.h"

namespace pw8::plugin::ui
{
    /// Header chrome shared across PLAY view modes (owned by MurmurRootEditor).
    struct SharedEditorChrome
    {
        PatchBrowserBar& patchBrowserBar;
        content::FavoritesStore& favoritesStore;
        PresetBrowserOverlay& presetBrowserOverlay;
    };

} // namespace pw8::plugin::ui
