#pragma once

namespace pw8::plugin::ui::layout
{
    /// Default PLAY editor frame: 16:9 landscape (fits beside a DAW mixer on 1080p/1440p).
    inline constexpr int kDefaultWidth = 1280;
    inline constexpr int kDefaultHeight = 720;
    inline constexpr int kMinWidth = 1120;
    inline constexpr int kMinHeight = 630;
    inline constexpr int kMaxWidth = 1920;
    inline constexpr int kMaxHeight = 1080;
    inline constexpr double kAspectRatio = 16.0 / 9.0;

    inline constexpr int kOuterMargin = 12;
    inline constexpr int kEngineRowHeight = 36;
    inline constexpr int kContextRowHeight = 28;
    inline constexpr int kViewModeRowHeight = 30;
    inline constexpr int kPatchFocusHeight = 132;
    inline constexpr int kPatchFocusBasicMinHeight = 280;
    inline constexpr int kTabRowHeight = 32;
    inline constexpr int kModBannerHeight = 28;
    inline constexpr int kSectionGap = 6;
    inline constexpr int kBlockGap = 8;

    /// Mod matrix overlay panel fills this fraction of the editor (panel itself stays 16:9).
    inline constexpr float kOverlayFillRatio = 0.88f;

    /// Right-side arpeggiator drawer width (PLAY UI P0).
    inline constexpr int kArpDrawerWidth = 420;

} // namespace pw8::plugin::ui::layout
