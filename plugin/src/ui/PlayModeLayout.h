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
    inline constexpr int kTopologyStripHeight = 52;
    inline constexpr int kTabRowHeight = 32;
    inline constexpr int kModBannerHeight = 28;
    inline constexpr int kSectionGap = 6;
    inline constexpr int kBlockGap = 8;

    /// Mod matrix overlay panel fills this fraction of the editor (panel itself stays 16:9).
    inline constexpr float kOverlayFillRatio = 0.88f;

    /// Right-side arpeggiator drawer width (PLAY UI P0).
    inline constexpr int kArpDrawerWidth = 420;

    /// Compact mode — fixed 320px width, resizable height.
    inline constexpr int kCompactWidth = 320;
    inline constexpr int kCompactMinHeight = 480;
    inline constexpr int kCompactMaxHeight = 1200;
    inline constexpr int kCompactDefaultHeight = 560;
    inline constexpr int kCompactOuterMargin = 8;
    inline constexpr int kCompactHeaderHeight = 28;
    inline constexpr int kCompactScopeSize = 148;
    inline constexpr int kCompactVolumeHeight = 72;
    inline constexpr int kCompactVolumeKnobWidth = 64;
    inline constexpr int kCompactBlockGap = 6;

} // namespace pw8::plugin::ui::layout
