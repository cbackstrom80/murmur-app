#pragma once

#include <algorithm>
#include <array>

namespace pw8::plugin::ui::layout
{
    /// Default PLAY editor frame: 16:9 landscape (fits beside a DAW mixer on 1080p/1440p).
    inline constexpr int kDefaultWidth = 1280;
    inline constexpr int kDefaultHeight = 720;
    inline constexpr int kMinWidth = 1120;
    /// Legacy floor — prefer `minimumRootHeight()` for view-aware sizing.
    inline constexpr int kMinHeight = 630;
    /// Allow expansion on hi-DPI / 4K / ultrawide monitors (no hard 1080p cap).
    inline constexpr int kMaxWidth = 3840;
    inline constexpr int kMaxHeight = 2160;
    /// Suggested aspect for default size only; resize is not locked to this ratio.
    inline constexpr double kAspectRatio = 16.0 / 9.0;

    inline constexpr int kOuterMargin = 16;
    /// Figma `obsidian-play-board` engine-grid-placeholder (22:37).
    inline constexpr int kEngineGridPadding = 16;
    inline constexpr int kEngineGridGap = 12;
    inline constexpr int kEngineGridHeaderHeight = 16;
    /// Vertical padding between grid header and cards (Figma engine-grid-placeholder).
    inline constexpr int kEngineGridContentPaddingY = 16;
    inline constexpr int kPolyphonyBadgeWidth = 112;
    inline constexpr int kLabChipWidth = 80;
    inline constexpr int kLabChipHeight = 28;
    inline constexpr int kEngineRowHeight = 36;
    inline constexpr int kContextRowHeight = 28;
    inline constexpr int kViewModeRowHeight = 30;
    inline constexpr int kCompactViewButtonWidth = 34;
    inline constexpr int kPatchFocusHeight = 132;
    inline constexpr int kTopologyStripHeight = 52;
    inline constexpr int kTabRowHeight = 32;
    inline constexpr int kModBannerHeight = 28;
    /// Major vertical stack gap in `obsidian-play-board` (22:2 flex gap = 12).
    inline constexpr int kSectionGap = 12;
    inline constexpr int kBlockGap = 8;

    /// Mod matrix overlay panel fills this fraction of the editor (panel itself stays 16:9).
    inline constexpr float kOverlayFillRatio = 0.88f;

    /// Full-screen arpeggiator lab (Figma `murmur-design-arp` / `murmur-arp-view` 4:1267).
    inline constexpr int kArpSidePanelWidth = 220;
    inline constexpr int kArpHeaderHeight = 36;
    /// Figma `murmur-arp-view` bottom bar (4:1623) = 44px.
    inline constexpr int kArpFooterHeight = 44;
    /// Figma `main-workspace` (4:1304) three-column row gap.
    inline constexpr int kDesignArpPageSectionGap = 12;
    inline constexpr int kDesignArpPageOuterMargin = 16;
    inline constexpr int kDesignArpPageMainWorkspaceHeight = 558;
    inline constexpr int kDesignArpPageCenterColumnWidth = 784;
    inline constexpr int kDesignArpPageSequencerCardHeight = 248;
    inline constexpr int kDesignArpPagePatternTuningHeight = 298;
    inline constexpr int kDesignArpPagePatternControlsWidth = 492;
    /// Figma `step-sequencer-row` (4:1353): 752px workspace, 16 × ~43.25px columns, 4px gaps.
    inline constexpr int kArpStepWorkspaceWidth = 752;
    inline constexpr int kArpStepColumnGap = 4;
    inline constexpr int kArpStepColumnWidth = 43;
    inline constexpr int kArpStepColumnWidthCompact = 28;
    inline constexpr int kArpStepPageSize = 16;
    inline constexpr int kArpStepToolRowHeight = 38;
    inline constexpr int kArpStepLaneWidth = 32;
    inline constexpr int kArpStepLaneHeight = 130;
    inline constexpr int kArpStepMetaHeight = 39;
    inline constexpr int kArpStepSequencerRowHeight = 191;
    inline constexpr int kArpSeqHeaderHeight = 20;
    inline constexpr int kArpStepToolbarButtonWidth = 30;
    inline constexpr int kArpStepCountLabelWidth = 56;
    inline constexpr int kArpModeButtonHeight = 23;
    inline constexpr int kArpModeButtonGap = 6;
    inline constexpr int kArpOctaveStripHeight = 22;
    inline constexpr int kArpHoldRowHeight = 32;
    inline constexpr int kArpSyncGridWidth = 150;
    inline constexpr int kArpSyncGridHeight = 52;
    inline constexpr int kArpSyncButtonWidth = 74;
    inline constexpr int kArpSyncButtonHeight = 18;
    inline constexpr int kArpTimingKnobWidth = 64;
    inline constexpr int kArpTimingKnobHeight = 55;
    inline constexpr int kArpTimingKnobDialSize = 32;
    inline constexpr int kArpRoutingRowHeight = 33;
    inline constexpr int kArpRoutingToggleWidth = 22;
    inline constexpr int kArpRoutingToggleHeight = 12;
    inline constexpr int kArpFooterPanicWidth = 73;
    inline constexpr int kArpFooterPanicHeight = 18;
    inline constexpr int kArpVelocityCurveCardWidth = 280;
    /// Figma `velocity-curve-card` (4:1558) height = 298.
    inline constexpr int kArpVelocityCurveCardHeight = 298;
    inline constexpr int kArpBottomPanelHeight = kArpVelocityCurveCardHeight;

    /// Legacy alias — ARP is now full-screen; kept for any stale references.
    inline constexpr int kArpDrawerWidth = kArpSidePanelWidth;

    /// Compact mode — Figma `murmur-compact-view` (`4:1134`), fixed 320×560 frame.
    inline constexpr int kCompactWidth = 320;
    inline constexpr int kCompactMinHeight = 480;
    inline constexpr int kCompactMaxHeight = 1200;
    inline constexpr int kCompactDefaultHeight = 560;
    /// Figma frame inset: top/left/right = 14, bottom = 30 (asymmetric).
    inline constexpr int kCompactOuterMargin = 14;
    inline constexpr int kCompactBottomMargin = 30;
    /// Figma `header-bar` (`50:252`) inside `4:1134` — maps to `MurmurChromeBar` in code.
    inline constexpr int kCompactHeaderHeight = 28;
    inline constexpr int kCompactMissionCardHeight = kCompactHeaderHeight + 38;
    inline constexpr int kCompactScopePanelHeight = 152;
    inline constexpr int kCompactScopeSize = 148;
    inline constexpr int kCompactScopePanelPadding = 12;
    inline constexpr int kCompactScopeHeaderHeight = 10;
    inline constexpr int kCompactScopeOscHeight = 110;
    inline constexpr int kCompactScopeHeaderGap = 8;
    inline constexpr int kCompactOutputMeterHeight = 6;
    inline constexpr int kCompactOutputMeterGap = 4;
    inline constexpr int kCompactFooterSystemHeight = 30;
    inline constexpr int kCompactModChipHeight = 12;
    inline constexpr int kCompactModChipGap = 4;
    inline constexpr int kCompactMacroPanelHeight = 158;
    inline constexpr int kCompactMacroPanelPadding = 12;
    inline constexpr int kCompactMacroHeaderHeight = 10;
    inline constexpr int kCompactMacroHeaderGap = 10;
    inline constexpr int kCompactMacroKnobSize = 36;
    inline constexpr int kCompactMacroKnobGap = 8;
    inline constexpr int kCompactMacroRowGap = 10;
    inline constexpr int kCompactMacroCellHeight = 52;
    /// Compact PLAY: two patch-prescribed performance KOINS (+ master volume = three mega knobs).
    inline constexpr int kCompactPerformanceKoinCount = 2;
    inline constexpr int kCompactMacroCount = kCompactPerformanceKoinCount;
    inline constexpr int kCompactMegaKnobDeckHeight = 120;
    inline constexpr int kCompactMegaKnobSize = 56;
    inline constexpr int kCompactOutputBlockHeight = 100;
    inline constexpr int kCompactMasterKnobSize = 48;
    inline constexpr int kCompactMasterKnobBlockWidth = 128;
    inline constexpr int kCompactStatsBlockWidth = 128;
    inline constexpr int kCompactVolumeHeight = kCompactOutputBlockHeight;
    inline constexpr int kCompactVolumeKnobWidth = kCompactMasterKnobSize;
    inline constexpr int kCompactBlockGap = 12;
    /// Figma footer chip widths (`4:1247`…`4:1253`): LFO1, ENV, SEQ, RAND.
    inline constexpr std::array<int, 4> kCompactModChipWidths = {28, 25, 25, 31};

    /// Alt Figma 8-engine VST grid (1440×1024 reference scaled to 16:9 PLAY frame).
    inline constexpr int kVstGridMinHeight = 320;
    /// Figma dashboard-strip-outer (22:188) height inside play-board.
    inline constexpr int kDashboardStripHeight = 168;
    inline constexpr int kDashboardInnerPadding = 8;
    inline constexpr int kDashboardFxFilterGap = 12;
    /// Figma global-filter-panel (22:376) width at 1280×720.
    inline constexpr int kDashboardFilterPanelWidth = 406;
    inline constexpr int kVstBottomBarHeight = 28;

    /// Figma `murmur-8-engine-vst` engine-card-1 (4:38) — full card anatomy (detail / 1440×1024).
    /// Play-board grid uses simplified stub cards (22:44): 86px tall, header + osc stub + level only.
    inline constexpr int kEngineCardPlayBoardHeight = 86;
    inline constexpr int kEngineCardPlayBoardHeaderHeight = 12;
    inline constexpr int kEngineCardPlayBoardMiddleRowHeight = 22;
    inline constexpr int kEngineCardPlayBoardOscStubWidth = 50;
    inline constexpr int kEngineCardPlayBoardKnobSize = 16;
    inline constexpr int kEngineCardPlayBoardKnobGap = 4;
    inline constexpr int kEngineCardPlayBoardRowGap = 10;
    inline constexpr int kEngineCardPlayBoardLevelRowHeight = 8;
    inline constexpr int kEngineCardPlayBoardTypeBadgeFontSize = 7;
    inline constexpr int kEngineCardPadding = 12;
    inline constexpr int kEngineCardCornerRadius = 8;
    inline constexpr int kEngineCardRowGap = 8;
    inline constexpr int kEngineCardHeaderHeight = 14;
    inline constexpr int kEngineCardLedSize = 6;
    inline constexpr int kEngineCardLedTitleGap = 6;
    inline constexpr int kEngineCardTitleFontSize = 10;
    inline constexpr int kEngineCardMixControlsWidth = 71;
    inline constexpr int kEngineCardOnButtonWidth = 25;
    inline constexpr int kEngineCardOnButtonHeight = 14;
    inline constexpr int kEngineCardSoloButtonWidth = 18;
    inline constexpr int kEngineCardMuteButtonWidth = 20;
    inline constexpr int kEngineCardMixButtonGap = 4;
    inline constexpr int kEngineCardPitchRowHeight = 52;
    inline constexpr int kEngineCardOscPickerWidth = 76;
    inline constexpr int kEngineCardOscPickerHeight = 52;
    inline constexpr int kEngineCardPitchKnobsGap = 8;
    inline constexpr int kEngineCardPitchKnobsYOffset = 5;
    inline constexpr int kEngineCardKnobWidth = 44;
    inline constexpr int kEngineCardKnobHeight = 42;
    inline constexpr int kEngineCardKnobDialSize = 28;
    inline constexpr int kEngineCardKnobGap = 4;
    inline constexpr int kEngineCardFilterEnvRowHeight = 62;
    inline constexpr int kEngineCardFilterKnobsWidth = 92;
    inline constexpr int kEngineCardFilterModeRowHeight = 16;
    inline constexpr int kEngineCardFilterModePillHeight = 12;
    inline constexpr int kEngineCardFilterModePadding = 2;
    inline constexpr int kEngineCardEnvelopeWidth = 64;
    inline constexpr int kEngineCardEnvelopeHeight = 50;
    inline constexpr int kEngineCardEnvelopeYOffset = 12;
    inline constexpr int kEngineCardFilterEnvGap = 8;
    inline constexpr int kEngineCardLevelRowHeight = 8;
    inline constexpr int kEngineCardLevelCaptionWidth = 13;
    inline constexpr int kEngineCardLevelCaptionGap = 8;
    inline constexpr int kEngineCardLevelSliderHeight = 6;
    inline constexpr int kEngineCardLevelValueWidth = 16;
    inline constexpr int kEngineCardLevelValueGap = 8;
    inline constexpr int kEngineCardMinHeight = 158;

    /// Figma `oscillator-picker` inside engine-card (4:51 / 13:4).
    inline constexpr int kEngineOscStripHeight = 16;
    inline constexpr int kEngineOscStripGridGap = 6;
    inline constexpr int kEngineOscContextGridHeight = 30;
    inline constexpr int kEngineOscCellWidth = 36;
    inline constexpr int kEngineOscCellHeight = 14;
    inline constexpr int kEngineOscCellGap = 2;
    inline constexpr int kEngineOscCellCornerRadius = 4;
    inline constexpr int kEngineOscTypePillFontSize = 7;

    /// Figma envelope-group mini ADSR (4:107).
    inline constexpr int kEngineAdsrPreviewHeight = 24;
    inline constexpr int kEngineAdsrLabelsHeight = 22;
    inline constexpr int kEngineAdsrTickWidth = 13;
    inline constexpr int kEngineAdsrTickTrackHeight = 12;
    inline constexpr int kEngineAdsrTickLabelFontSize = 8;

    /// Figma vst-top-bar (22:3) — single chrome row inside obsidian-play-board.
    inline constexpr int kObsidianChromeHeight = 54;
    inline constexpr int kObsidianChromePaddingX = 16;
    inline constexpr int kObsidianBrandWidth = 166;
    inline constexpr int kObsidianPresetBrowserWidth = 320;
    inline constexpr int kObsidianMasterKnobSize = 28;
    /// View-mode strip overlays inside unified chrome (compact mode uses standalone bar).
    inline constexpr int kVstTopBarHeight = 30;

    /// Figma murmur-play-fx-rack (15:478) inside dashboard-strip-outer (168px).
    inline constexpr int kFxChainFlowHeight = 52;
    inline constexpr int kFxChainEditorGap = 6;
    inline constexpr int kFxSlotEditorHeight = 94;
    inline constexpr int kDashboardGlobalFilterKnobSize = 36;

    inline constexpr int kVstBottomBarPaddingX = 16;

    /// Figma `murmur-desktop-play-mode` (36:4) — Basic performance screen (distinct from obsidian-play-board 22:2).
    /// Canonical frame height is 960px (includes inline header in Figma; chrome is external in code).
    inline constexpr int kDesktopPlayModeFrameHeight = 720;
    inline constexpr int kDesktopPlayModeOuterMargin = 20;
    inline constexpr int kDesktopPlayModeMasterEnvelopeSectionHeight = 220;
    inline constexpr int kDesktopPlayModeSectionGap = 16;
    inline constexpr int kDesktopPlayModeContentWidth = 1240;
    /// Legacy integrated top-bar (36:5); chrome now lives in unified `MurmurChromeBar` (`39:142`, 44px).
    inline constexpr int kDesktopPlayModeTopBarHeight = 72;
    inline constexpr int kDesktopPlayModeUnifiedChromeHeight = 44;
    inline constexpr int kDesktopPlayModeTopBarPaddingX = 24;
    inline constexpr int kDesktopPlayModeTopBarCornerRadius = 12;
    inline constexpr int kDesktopPlayModeBrandLogoSize = 32;
    inline constexpr int kDesktopPlayModeBrandGroupGap = 14;
    inline constexpr int kDesktopPlayModePresetBrowserWidth = 520;
    inline constexpr int kDesktopPlayModePresetBrowserHeight = 52;
    inline constexpr int kDesktopPlayModePresetNavButtonWidth = 48;
    inline constexpr int kDesktopPlayModeTempoBoxWidth = 90;
    inline constexpr int kDesktopPlayModeTempoBoxHeight = 44;
    inline constexpr int kDesktopPlayModeMasterOutWidth = 120;
    inline constexpr int kDesktopPlayModeMasterOutHeight = 24;
    inline constexpr int kDesktopPlayModeMasterLedTrackHeight = 10;
    inline constexpr int kDesktopPlayModeTransportGap = 20;
    inline constexpr int kDesktopPlayModeOscilloscopeHeight = 180;
    inline constexpr int kDesktopPlayModeOscilloscopeCornerRadius = 8;
    inline constexpr int kDesktopPlayModeScopeMetadataInsetX = 16;
    inline constexpr int kDesktopPlayModeScopeMetadataInsetY = 12;
    inline constexpr int kDesktopPlayModeScopeWavePaddingX = 48;
    inline constexpr int kDesktopPlayModeScopeMetadataHeight = 46;
    inline constexpr int kDesktopPlayModeScopePlotPaddingBottom = 8;
    inline constexpr int kDesktopPlayModeMasterEnvelopeCompactRowHeight = 27;
    inline constexpr int kDesktopPlayModeMasterEnvelopeCompactRowGap = 4;
    inline constexpr int kDesktopPlayModeMasterEnvelopeCompactKnobSize = 40;
    inline constexpr int kDesktopPlayModePerformanceDeckHeight = 280;
    inline constexpr int kDesktopPlayModePerformanceDeckPadding = 24;
    inline constexpr int kDesktopPlayModeMacrosHeaderHeight = 13;
    inline constexpr int kDesktopPlayModeKnobsRowHeight = 148;
    inline constexpr int kDesktopPlayModeMacroKnobWidth = 120;
    inline constexpr int kDesktopPlayModeMacroKnobGap = 33;
    inline constexpr int kDesktopPlayModeMacroKnobTouchSize = 92;
    inline constexpr int kDesktopPlayModeBottomBarHeight = 64;
    inline constexpr int kDesktopPlayModeBottomBarPaddingX = 24;
    inline constexpr int kDesktopPlayModeEngineIndicatorWidth = 48;
    inline constexpr int kDesktopPlayModeEngineIndicatorGap = 6;
    inline constexpr int kDesktopPlayModeStageActionHeight = 33;
    inline constexpr int kDesktopPlayModeLatchButtonWidth = 127;
    inline constexpr int kDesktopPlayModePanicButtonWidth = 101;
    inline constexpr int kDesktopPlayModeStageActionGap = 16;
    inline constexpr int kDesktopPlayModeMacroCount = 8;
    /// Compact master envelope strip in desktop PLAY (between scope and macro deck).
    inline constexpr int kDesktopPlayModeMasterEnvelopePlayHeight = 168;
    /// Desktop PLAY content stack: scope + master envelope + macro deck + bottom bar.
    inline constexpr int kDesktopPlayModePureContentHeight =
        kDesktopPlayModeOscilloscopeHeight + kDesktopPlayModeSectionGap + kDesktopPlayModeMasterEnvelopePlayHeight
        + kDesktopPlayModeSectionGap + kDesktopPlayModePerformanceDeckHeight + kDesktopPlayModeSectionGap
        + kDesktopPlayModeBottomBarHeight;

    inline constexpr int kChromeSubNavDesktopPlayWidth = 52;
    inline constexpr int kChromeSubNavPlayBoardWidth = 44;

    /// Figma `ipad-play-view` (4:2472) — alternate PLAY layout (desktop content area).
    inline constexpr int kIpadPlayUpperDeckHeight = 220;
    inline constexpr int kIpadPlayUpperDeckGap = 12;
    inline constexpr int kIpadPlayMasterDeckWidth = 400;
    inline constexpr int kIpadPlayMasterKnobSize = 56;
    inline constexpr int kIpadPlayMasterStripHeight = 120;
    inline constexpr int kIpadPlayMasterStripPadding = 12;
    inline constexpr int kIpadPlayMasterStripHeaderHeight = 16;
    inline constexpr int kIpadPlayMasterStripCurveWidth = 168;
    inline constexpr int kIpadPlayMasterStripCurveHeight = 56;
    inline constexpr int kIpadPlayMasterStripColumnGap = 8;
    inline constexpr int kIpadPlayMasterStripKnobRowHeight = 72;
    inline constexpr int kIpadPlayMacrosDeckHeight = 180;
    inline constexpr int kIpadPlayMacroCount = 6;
    inline constexpr int kIpadPlayMacroKnobWidth = 120;
    inline constexpr int kIpadPlayMacroKnobGap = 24;
    inline constexpr int kIpadPlaySectionGap = 12;
    inline constexpr float kIpadPlaySectionTitleSize = 12.0f;
    inline constexpr float kIpadPlayCaptionSize = 10.5f;
    inline constexpr float kIpadPlayLabelSize = 11.0f;
    /// Figma `ipad-play-view` footer pill strip (`4:2638`).
    inline constexpr int kIpadPlayFooterPillHeight = 38;
    inline constexpr int kIpadPlayFooterPillGap = 4;
    inline constexpr int kIpadPlayFooterPillStripWidth = 315;
    inline constexpr int kIpadPlayFooterPillPlayWidth = 60;
    inline constexpr int kIpadPlayFooterPillDesignWidth = 71;
    inline constexpr int kIpadPlayFooterPillVocWidth = 56;
    inline constexpr int kIpadPlayFooterPillArpWidth = 54;
    inline constexpr int kIpadPlayFooterPillLfoWidth = 52;

    /// Figma `murmur-basic-view` (`86:4`) — envelope hero + performance sidebar.
    inline constexpr int kMurmurBasicViewOuterMargin = 16;
    inline constexpr int kMurmurBasicViewMainBodyGap = 12;
    inline constexpr int kMurmurBasicViewEnvelopePanelWidth = 680;
    inline constexpr int kMurmurBasicViewPerformanceSidebarWidth = 556;
    inline constexpr int kMurmurBasicViewBottomBarHeight = 40;
    inline constexpr int kMurmurBasicViewMacroCount = 4;
    inline constexpr int kMurmurBasicViewMacroKnobSize = 88;
    inline constexpr int kMurmurBasicViewPortamentoKnobSize = 56;
    inline constexpr int kMurmurBasicViewVuMeterHeight = 108;
    inline constexpr int kMurmurBasicViewSidebarPadding = 16;

    enum class IpadFooterPill
    {
        Play = 0,
        Design,
        Voc,
        Arp,
        Lfo,
    };

    enum class PlayViewMode
    {
        /// Figma `murmur-desktop-play-mode` (`36:4`): scope + 8-macro deck + bottom bar.
        Desktop,
        /// Legacy desktop layout with master envelope strip (pre-36:4 experiment).
        Basic,
        /// Legacy alias — UI routes to `EditorMode::Design` instead of showing this layout.
        Advanced,
        Compact,
    };

    [[nodiscard]] inline bool isDesktopPlayLayout(PlayViewMode mode) noexcept
    {
        return mode == PlayViewMode::Desktop || mode == PlayViewMode::Basic;
    }

    [[nodiscard]] inline bool isPureDesktopPlayLayout(PlayViewMode mode) noexcept
    {
        return mode == PlayViewMode::Desktop;
    }

    enum class PlayLabOverlay
    {
        None,
        Vocoder,
        Quasar,
        Motion,
        DualLfo,
        Wavetable,
    };

    enum class EditorMode
    {
        Play,
        Design,
    };

    /// Design sub-nav targets (Figma prototype nav map — `murmur-design-engine` `37:787` header `39:19`).
    enum class DesignSubPage
    {
        Engine,
        Arp,
        Vocoder,
        Fx,
        ModMatrix,
        Browse,
        Wavetable,
        DualLfo,
        Morph,
        FilterLab,
        DynamicsLab,
        GenerativeLab,
        UtilityPeaks,
        EnvelopeSegments,
    };

    /// Figma unified header-bar — `39:158` compact / `39:142` play / `39:2` design.
    /// Matches Figma `header-bar` (`50:252`) height inside `4:1134`.
    inline constexpr int kChromeBarHeightCompact = 28;
    inline constexpr int kChromeBarHeightPlay = 44;
    inline constexpr int kChromeBarHeightDesign = 60;
    /// Root frame height for pure `36:4` (chrome external in code).
    inline constexpr int kDesktopPlayModePureRootHeight =
        kDesktopPlayModeOuterMargin * 2 + kChromeBarHeightPlay + kDesktopPlayModeSectionGap
        + kDesktopPlayModePureContentHeight;
    inline constexpr int kChromeBarPaddingX = 16;
    inline constexpr int kChromeBarPaddingY = 10;
    inline constexpr int kChromeBarTopRowPaddingY = 10;
    inline constexpr int kChromeBarDesignTopRowHeight = 23;
    inline constexpr int kChromeBarDesignSubNavRowHeight = 22;
    inline constexpr int kChromeBarDesignSubNavGap = 4;
    inline constexpr int kChromeBarTopRowHeight = 24;
    /// Reserved left column for whale lockup (85px asset + margin). Nav begins after this + `kChromeNavSectionGap`.
    inline constexpr int kChromeBarBrandWidth = 96;
    inline constexpr int kChromeBarLogoInset = 2;
    /// Max painted lockup height — keeps logo inside the chrome row beside nav labels.
    inline constexpr int kChromeBarPlayLogoMaxHeight = 22;
    inline constexpr int kChromeBarDesignLogoMaxHeight = 17;
    inline constexpr int kChromeBarScoreLineWidth = 1;
    inline constexpr int kChromeBarScoreLineHeight = 24;
    inline constexpr int kChromeBarLedSize = 6;
    inline constexpr int kChromeBarLedGap = 4;
    inline constexpr int kChromeViewLedGap = 16;
    /// Gap between view-mode tabs and the following chrome section (divider / sub-nav).
    inline constexpr int kChromeNavSectionGap = 8;
    inline constexpr int kChromeSectionLedGap = 12;
    inline constexpr int kChromeBarSubNavGap = 6;
    inline constexpr int kChromeBarSubNavHeight = 18;
    inline constexpr int kChromeBarSubNavTabGap = 12;
    inline constexpr int kChromeViewTabHeight = 19;
    inline constexpr int kChromeViewTabCompactWidth = 71;
    inline constexpr int kChromeViewTabPlayWidth = 48;
    inline constexpr int kChromeViewTabDesignWidth = 58;
    inline constexpr int kChromeViewTabGap = 4;
    /// Legacy Figma estimates — runtime layout measures label widths instead.
    inline constexpr int kChromeViewTabCmpWidth = 52;
    inline constexpr int kChromeViewTabPlyWidth = 36;
    inline constexpr int kChromeViewTabDsnWidth = 48;
    inline constexpr int kChromeViewTabCompactHeight = 7;
    inline constexpr int kChromeViewTabCompactGap = 6;
    inline constexpr int kChromeSubNavEngineWidth = 44;
    inline constexpr int kChromeSubNavArpWidth = 28;
    inline constexpr int kChromeSubNavVocoderWidth = 28;
    inline constexpr int kChromeSubNavFxWidth = 22;
    inline constexpr int kChromeSubNavModMatrixWidth = 36;
    inline constexpr int kChromeSubNavBrowseWidth = 58;
    inline constexpr int kChromeSubNavWavetableWidth = 72;
    inline constexpr int kChromeSubNavDualLfoWidth = 62;
    inline constexpr int kChromeSubNavFilterLabWidth = 44;
    inline constexpr int kChromeSubNavDynamicsLabWidth = 44;
    inline constexpr int kChromeSubNavGenerativeLabWidth = 36;
    inline constexpr int kChromeSubNavUtilityPeaksWidth = 44;
    inline constexpr int kChromeSubNavEnvelopeSegmentsWidth = 36;
    inline constexpr int kChromePresetChevronWidth = 12;
    inline constexpr int kChromeMasterKnobSize = 28;
    inline constexpr int kChromePresetDisplayWidth = 260;

    [[nodiscard]] inline int chromeBarNavStartX() noexcept
    {
        return kChromeBarPaddingX + kChromeBarBrandWidth + kChromeNavSectionGap;
    }

    /// Embedded design lab panel header row (aligns toward design chrome 62px).
    inline constexpr int kDesignLabPanelHeaderHeight = 56;

    [[nodiscard]] inline int chromeBarHeight(layout::EditorMode mode, bool compactWindow) noexcept
    {
        if (compactWindow)
            return kChromeBarHeightCompact;
        if (mode == EditorMode::Design)
            return kChromeBarHeightDesign;
        return kChromeBarHeightPlay;
    }

    /// Minimum PLAY content height so footer + primary sections stay visible together.
    [[nodiscard]] inline int minimumPlayContentHeight(PlayViewMode mode) noexcept
    {
        switch (mode)
        {
            case PlayViewMode::Desktop:
                return kDesktopPlayModePureContentHeight;
            case PlayViewMode::Basic:
                return kDesktopPlayModeOscilloscopeHeight + kDesktopPlayModeSectionGap
                       + kDesktopPlayModeMasterEnvelopeSectionHeight + kDesktopPlayModeSectionGap
                       + kDesktopPlayModePerformanceDeckHeight + kDesktopPlayModeSectionGap
                       + kDesktopPlayModeBottomBarHeight;
            case PlayViewMode::Advanced:
                return kTabRowHeight + kSectionGap + kVstBottomBarHeight + kSectionGap + kDashboardStripHeight
                       + kSectionGap + kVstGridMinHeight;
            case PlayViewMode::Compact:
                return kCompactDefaultHeight - kCompactOuterMargin - kCompactBottomMargin;
        }
        return kDesktopPlayModePureContentHeight;
    }

    /// Root editor height that keeps unified chrome + PLAY footer on screen at once.
    [[nodiscard]] inline int minimumPlayRootHeight(PlayViewMode mode) noexcept
    {
        const int insets = kDesktopPlayModeOuterMargin * 2;
        const int computed = insets + chromeBarHeight(EditorMode::Play, mode == PlayViewMode::Compact)
                             + kDesktopPlayModeSectionGap + minimumPlayContentHeight(mode);
        if (mode == PlayViewMode::Desktop)
            return std::min(computed, kDefaultHeight);
        return computed;
    }

    /// Figma `murmur-design-engine` (37:787) — ENGINE sub-page: 8 cards, no FX rack.
    inline constexpr int kDesignModeV2OuterMargin = 16;
    inline constexpr int kDesignModeV2SectionGap = 12;
    inline constexpr int kDesignModeV2HeaderHeight = kChromeBarHeightDesign;
    inline constexpr int kDesignModeV2HeaderPaddingX = 16;
    inline constexpr int kDesignModeV2NavTabHeight = 24;
    inline constexpr int kDesignModeV2NavTabGap = 4;
    inline constexpr int kDesignModeV2PresetSelectorWidth = 240;
    inline constexpr int kDesignModeV2PresetSelectorHeight = 32;
    inline constexpr int kDesignModeV2MasterKnobSize = 28;
    inline constexpr int kDesignModeV2GridSectionHeight = 452;
    inline constexpr int kDesignModeV2MasterEnvelopePanelHeight = 180;
    inline constexpr int kDesignModeV2MasterEnvelopePadding = 16;
    inline constexpr int kDesignModeV2MasterEnvelopeHeaderHeight = 16;
    inline constexpr int kDesignModeV2MasterEnvelopeContentGap = 12;
    inline constexpr int kDesignModeV2MasterEnvelopeCurveWidth = 520;
    inline constexpr int kDesignModeV2MasterEnvelopePlotHeight = 80;
    inline constexpr int kDesignModeV2MasterEnvelopeKnobSize = 36;
    inline constexpr int kDesignModeV2MasterEnvelopeKnobColumnWidth = 150;
    inline constexpr int kDesignModeV2MasterEnvelopeControlRowHeight = 120;
    inline constexpr int kDesignModeV2MasterEnvelopeControlRowGap = 10;
    inline constexpr int kDesignModeV2StatusBarHeight = 40;
    /// Figma `murmur-design-engine` (`37:787`) — 1280×800 target frame.
    inline constexpr int kDesignEngineHeight = 800;

    [[nodiscard]] inline int minimumDesignRootHeight() noexcept
    {
        return kDesignEngineHeight;
    }

    [[nodiscard]] inline int defaultPlayRootHeight(PlayViewMode mode) noexcept
    {
        if (mode == PlayViewMode::Compact)
            return kCompactDefaultHeight;
        return std::max(kDefaultHeight, minimumPlayRootHeight(mode));
    }

    /// Clamp user resize to view minimums without forcing a fixed frame height.
    [[nodiscard]] inline int clampRootHeight(int height, EditorMode editorMode, PlayViewMode playMode,
                                             bool compactWindow) noexcept
    {
        const int minH = compactWindow ? kCompactMinHeight
                                       : (editorMode == EditorMode::Design ? minimumDesignRootHeight()
                                                                           : minimumPlayRootHeight(playMode));
        return std::clamp(height, minH, kMaxHeight);
    }

    inline constexpr int kDesignModeV2GridRowHeight = 220;
    inline constexpr int kDesignModeV2GridRowGap = 12;
    inline constexpr int kDesignModeV2GridColGap = 12;
    inline constexpr int kDesignModeV2EngineCardWidth = 303;
    inline constexpr int kDesignModeV2EngineCardHeight = 220;
    inline constexpr int kDesignModeV2CardCornerRadius = 6;
    inline constexpr int kDesignModeV2StatusBarPaddingX = 16;
    /// Footer lab chips — secondary shortcuts only (primary labs live in header sub-nav).
    inline constexpr int kDesignStatusBarLabChipWidth = 58;
    inline constexpr int kDesignStatusBarLabChipGap = 4;
    inline constexpr int kDesignStatusBarPanicWidth = 68;
    inline constexpr int kDesignStatusBarZoneGap = 12;
    inline constexpr int kDesignModeV2CardPadding = 12;
    inline constexpr int kDesignModeV2CardRowGap = 10;
    inline constexpr int kDesignModeV2CardRowGapAfterSubPicker = 12;
    inline constexpr int kDesignModeV2CardRowGapBeforeKnobs = 8;
    inline constexpr int kDesignModeV2CardHeaderHeight = 14;
    inline constexpr int kDesignModeV2TypeStripHeight = 14;
    inline constexpr int kDesignModeV2SubPickerHeight = 20;
    inline constexpr int kDesignModeV2SubPickerPillHeight = 14;
    inline constexpr int kDesignModeV2SubPickerArrowWidth = 22;
    inline constexpr int kDesignModeV2ContextVisualizerHeight = 80;
    inline constexpr int kDesignModeV2ContextVisualizerMinHeight = 40;
    inline constexpr int kDesignModeV2OscillatorPickerHeight =
        kDesignModeV2TypeStripHeight + kDesignModeV2CardRowGap + kDesignModeV2SubPickerHeight
        + kDesignModeV2CardRowGapAfterSubPicker + kDesignModeV2ContextVisualizerHeight;
    inline constexpr int kDesignModeV2KnobsEnvelopeRowHeight = 48;
    inline constexpr int kDesignModeV2CardKnobContainerWidth = 44;
    inline constexpr int kDesignModeV2CardKnobContainerGap = 4;
    inline constexpr int kDesignModeV2CardKnobDialSize = 28;
    inline constexpr int kDesignModeV2LevelRowHeight = 10;
    inline constexpr int kDesignModeV2EnvelopeWidth = 68;
    inline constexpr int kDesignModeV2EnvelopeHeight = 40;
    inline constexpr int kDesignModeV2EnvelopeYOffset = 8;

    /// Figma `murmur-fx-card-browser` (152:4) — design FX card landing grid.
    inline constexpr int kDesignFxCardBrowserSubHeaderHeight = 34;
    inline constexpr int kDesignFxCardBrowserStatusBarHeight = 46;
    inline constexpr int kDesignFxCardWidth = 230;
    inline constexpr int kDesignFxCardHeight = 280;
    inline constexpr int kDesignFxCardGap = 12;
    inline constexpr int kDesignFxCardPadding = 12;
    inline constexpr int kDesignFxCardSectionGap = 10;
    inline constexpr int kDesignFxCardRadius = 8;
    inline constexpr int kDesignFxCardAccentStripHeight = 3;
    inline constexpr int kDesignFxCardAccentStripWidth = 206;
    inline constexpr int kDesignFxCardMiniVisHeight = 50;
    inline constexpr int kDesignFxCardMiniVisWidth = 200;
    inline constexpr int kDesignFxCardMiniKnobSize = 24;
    inline constexpr int kDesignFxCardMiniKnobColWidth = 48;
    inline constexpr int kDesignFxCardToggleWidth = 22;
    inline constexpr int kDesignFxCardToggleHeight = 14;
    inline constexpr int kDesignFxViewToggleWidth = 100;
    inline constexpr int kDesignFxViewToggleHeight = 26;
    inline constexpr int kDesignFxCardColumns = 5;
    inline constexpr int kDesignFxCardBrowserCount = 10;

    /// Figma `murmur-design-fx` (35:4) — standalone design FX rack page.
    inline constexpr int kDesignFxPageSectionGap = 12;
    inline constexpr int kDesignFxPageSignalChainLabelHeight = 11;
    inline constexpr int kDesignFxPageSignalChainLabelGap = 6;
    inline constexpr int kDesignFxPageSignalChainPipelineHeight = 94;
    inline constexpr int kDesignFxPageSignalChainSectionHeight =
        kDesignFxPageSignalChainLabelGap + kDesignFxPageSignalChainLabelHeight + kDesignFxPageSignalChainLabelGap
        + kDesignFxPageSignalChainPipelineHeight;
    inline constexpr int kDesignFxPageChipWidth = 82;
    inline constexpr int kDesignFxPageChipHeight = 82;
    inline constexpr int kDesignFxPageChipPadding = 6;
    inline constexpr int kDesignFxPageFlowConnectorWidth = 12;
    inline constexpr int kDesignFxPageTerminalInWidth = 35;
    inline constexpr int kDesignFxPageTerminalOutWidth = 44;
    inline constexpr int kDesignFxPageMiddleHeight = 419;
    /// Figma `murmur-fx-*` (63:8+) — full-width hero detail (no overview sidebar).
    inline constexpr int kDesignFxPageDetailFullWidth = 1248;
    inline constexpr int kDesignFxPageDetailWidth = kDesignFxPageDetailFullWidth;
    inline constexpr int kDesignFxPageDetailHeight = 360;
    inline constexpr int kDesignFxPageDetailPadding = 16;
    inline constexpr int kDesignFxPageDetailHeaderHeight = 18;
    inline constexpr int kDesignFxPageDetailHeaderGap = 14;
    inline constexpr int kDesignFxPageDetailBodyHeight = 296;
    inline constexpr int kDesignFxPageDetailControlsWidth = 280;
    inline constexpr int kDesignFxPageDetailHeroWidth = 448;
    inline constexpr int kDesignFxPageEqSidebarWidth = 120;
    inline constexpr int kDesignFxPageEqGraphHeight = 220;
    inline constexpr int kDesignFxPageEqParamStripHeight = 29;
    inline constexpr int kDesignFxPageDetailActiveToggleWidth = 48;
    inline constexpr int kDesignFxPageDetailActiveToggleHeight = 16;
    inline constexpr int kDesignFxPageDetailControlsVizGap = 16;
    inline constexpr int kDesignFxPageDetailKnobGridHeight = 174;
    inline constexpr int kDesignFxPageDetailKnobWidth = 64;
    inline constexpr int kDesignFxPageDetailKnobHeight = 58;
    inline constexpr int kDesignFxPageDetailKnobColGap = 44;
    inline constexpr int kDesignFxPageDetailKnobRowGap = 58;
    inline constexpr int kDesignFxPageDetailModeStripHeight = 26;
    inline constexpr int kDesignFxPageOverviewWidth = 380;
    inline constexpr int kDesignFxPageOverviewHeight = 360;
    inline constexpr int kDesignFxPageOverviewPadding = 12;
    inline constexpr int kDesignFxPageOverviewRowHeight = 24;
    inline constexpr int kDesignFxPageOverviewRowGap = 4;
    inline constexpr int kDesignFxPageRoutingBarHeight = 54;
    inline constexpr int kDesignFxPageRoutingPaddingX = 16;
    inline constexpr int kDesignFxPageSlotCount = 12;
    inline constexpr int kDesignFxPageDetailKnobDialSize = 32;

    /// Figma `murmur-master-quasar-binaural` (102:4)
    inline constexpr int kQuasarPanelHeaderHeight = 44;
    inline constexpr int kQuasarBinauralFieldHeight = 318;
    inline constexpr int kQuasarPrimaryKnobRowHeight = 105;
    inline constexpr int kQuasarBottomCardHeight = 141;
    inline constexpr int kQuasarTelemetryBarHeight = 32;
    inline constexpr int kQuasarPrimaryKnobCellWidth = 80;
    inline constexpr int kQuasarPrimaryKnobDialSize = 44;

    /// Figma `murmur-design-mod-matrix` (27:265) — standalone design mod matrix page.
    inline constexpr int kDesignModMatrixPageOuterMargin = 12;
    inline constexpr int kDesignModMatrixPageSectionGap = 10;
    inline constexpr int kDesignModMatrixPageStatusBarHeight = 40;
    inline constexpr int kDesignModMatrixPageLeftWidth = 966;
    inline constexpr int kDesignModMatrixPageSidebarWidth = 280;
    inline constexpr int kDesignModMatrixPageSidebarGap = 10;
    inline constexpr int kDesignModMatrixPageGridCardHeight = 306;
    inline constexpr int kDesignModMatrixPageRoutesCardHeight = 260;
    inline constexpr int kDesignModMatrixPageCardGap = 10;
    inline constexpr int kDesignModMatrixPageCardPadding = 12;
    inline constexpr int kDesignModMatrixPageCardHeaderHeight = 14;
    inline constexpr int kDesignModMatrixPageCardInnerGap = 8;
    inline constexpr int kDesignModMatrixPageGridCanvasPadding = 8;
    inline constexpr int kDesignModMatrixPageGridHeaderHeight = 14;
    inline constexpr int kDesignModMatrixPageSourceColumnWidth = 70;
    inline constexpr int kDesignModMatrixPageDestColumnWidth = 105;
    inline constexpr int kDesignModMatrixPageGridColumnGap = 2;
    inline constexpr int kDesignModMatrixPageGridRowHeight = 20;
    inline constexpr int kDesignModMatrixPageCellDialSize = 16;
    inline constexpr int kDesignModMatrixPageCellEmptySize = 6;
    inline constexpr int kDesignModMatrixPageRouteRowHeight = 28;
    inline constexpr int kDesignModMatrixPageRouteRowGap = 4;
    inline constexpr int kDesignModMatrixPageRouteSourceWidth = 80;
    inline constexpr int kDesignModMatrixPageRouteDestWidth = 130;
    inline constexpr int kDesignModMatrixPageRouteDepthTrackWidth = 522;
    inline constexpr int kDesignModMatrixPageSidebarPadding = 12;
    inline constexpr int kDesignModMatrixPageQuickConfigPillWidth = 32;
    inline constexpr int kDesignModMatrixPageQuickConfigPillHeight = 18;
    inline constexpr int kDesignModMatrixPageSourceRowCount = 11;
    inline constexpr int kDesignModMatrixPageDestColumnCount = 8;

    /// Figma `murmur-vocoder-lab` (15:4).
    inline constexpr int kDesignVocoderPageOuterMargin = 16;
    inline constexpr int kDesignVocoderPageSectionGap = 12;
    inline constexpr int kDesignVocoderSignalDiagramHeight = 110;
    inline constexpr int kDesignVocoderSignalDiagramPadding = 12;
    inline constexpr int kDesignVocoderSignalCanvasHeight = 60;
    inline constexpr int kDesignVocoderControlsPanelWidth = 680;
    inline constexpr int kDesignVocoderFooterHeight = 32;
    inline constexpr int kDesignVocoderBandCount = 16;
    inline constexpr int kDesignVocoderBandColumnWidth = 24;
    inline constexpr int kDesignVocoderBandBarWidth = 10;
    inline constexpr int kDesignVocoderBandGraphHeight = 210;
    inline constexpr int kDesignVocoderControlsPadding = 16;
    inline constexpr int kDesignVocoderKnobDiameter = 44;
    inline constexpr int kDesignVocoderChainRoutingBarHeight = 38;
    inline constexpr int kDesignVocoderCardPadding = 12;

    /// Figma `murmur-wavetable-editor` (27:709).
    inline constexpr int kDesignWavetablePageOuterMargin = 12;
    inline constexpr int kDesignWavetablePageSectionGap = 10;
    inline constexpr int kDesignWavetableSubtitleBarHeight = 24;
    inline constexpr int kDesignWavetableLeftColumnWidth = 240;
    inline constexpr int kDesignWavetableRightColumnWidth = 280;
    inline constexpr int kDesignWavetableEditorGap = 10;
    inline constexpr int kDesignWavetableHarmonicEditorWidth = 256;
    inline constexpr int kDesignWavetableHarmonicEditorHeight = 336;
    inline constexpr int kDesignWavetableFrameStripHeight = 100;
    inline constexpr int kDesignWavetableFrameMiniWidth = 70;
    inline constexpr int kDesignWavetableFrameMiniHeight = 50;
    inline constexpr int kDesignWavetableFrameStripCount = 8;
    inline constexpr int kDesignWavetableFooterHeight = 40;

    /// Figma `murmur-mi-ui-play-filter-blades` — `blades-section-panel` (`89:131`).
    inline constexpr int kPlayBladesSectionPadding = 12;
    inline constexpr int kPlayBladesSectionGap = 8;
    inline constexpr int kPlayBladesHeaderRowHeight = 14;
    inline constexpr int kPlayBladesKnobsRowHeight = 68;
    inline constexpr int kPlayBladesKnobDialSize = 44;
    inline constexpr int kPlayBladesKnobCount = 7;
    inline constexpr int kPlayBladesSectionHeight = 104;
    inline constexpr int kPlayBladesKnobLabelGap = 4;

    /// Figma `murmur-mi-ui-component-blades-routing-diagram` (`89:246`).
    inline constexpr int kFilterRoutingWireframeStateRowHeight = 44;
    inline constexpr int kFilterRoutingWireframeStateGap = 6;
    inline constexpr int kFilterRoutingWireframeLabelWidth = 50;

    /// Figma `morph-timeline-panel` (`89:736`) inside `murmur-mi-ui-play-morph-timeline` (`89:641`).
    inline constexpr int kMorphTimelinePanelHeight = 136;
    /// Figma `murmur-master-motion-lab` (`94:4715`) — stacked overlay layout budgets.
    inline constexpr int kMasterMotionLabEnvelopeHeroHeight = 148;
    inline constexpr int kMasterMotionLabMorphHeight = 96;
    inline constexpr int kMasterMotionLabLfoKnobsRowHeight = 84;
    inline constexpr int kMasterMotionLabLfoSyncRowHeight = 22;
    inline constexpr int kMasterMotionLabLfoScopeMinHeight = 64;
    inline constexpr int kMasterMotionLabFooterHeight = 32;
    inline constexpr int kMasterMotionLabSectionGap = 6;
    inline constexpr int kMasterMotionLabSegmentStripHeight = 36;
    inline constexpr int kMasterMotionLabSegmentDotCount = 6;
    inline constexpr int kMorphTimelinePanelPadding = 12;
    inline constexpr int kMorphTimelineHeaderWidth = 260;
    inline constexpr int kMorphTimelineTrackColumnWidth = 772;
    inline constexpr int kMorphTimelineKnobBlockWidth = 160;
    inline constexpr int kMorphTimelineStripHeight = 52;
    inline constexpr int kMorphTimelineGradientTrackHeight = 16;
    inline constexpr int kMorphTimelineGradientTrackInsetX = 12;
    inline constexpr int kMorphTimelinePlayheadSize = 16;
    inline constexpr int kMorphTimelineChipHeight = 17;
    inline constexpr int kMorphTimelineChipGap = 6;
    inline constexpr int kMorphTimelineKeyframeTickHeight = 8;
    inline constexpr int kMorphTimelineMorphKnobDialSize = 64;

    /// Figma `master-envelope-section` (`82:83`) embedded in play-morph-timeline.
    inline constexpr int kMasterEnvelopeSectionHeight = 220;
    inline constexpr int kMasterEnvelopeSectionWidth = 1240;

    /// Figma `murmur-mi-ui-design-morph-editor` (`89:953`).
    inline constexpr int kDesignMorphEditorLeftColumnWidth = 264;
    inline constexpr int kDesignMorphEditorCenterColumnWidth = 736;
    inline constexpr int kDesignMorphEditorRightColumnWidth = 280;
    inline constexpr int kDesignMorphEditorKeyframeCardHeight = 44;
    inline constexpr int kDesignMorphEditorOverrideRowHeight = 28;
    inline constexpr int kDesignMorphEditorColorSwatchSize = 14;

    /// Figma `murmur-mi-ui-play-focus-morph-hub` (`94:5038`).
    inline constexpr int kPlayFocusMorphHubMiniStripHeight = 48;

    /// Figma `murmur-mi-ui-design-filter-lab` (`89:313`).
    inline constexpr int kDesignFilterLabPageOuterMargin = 16;
    inline constexpr int kDesignFilterLabPageSectionGap = 12;
    inline constexpr int kDesignFilterLabLeftColumnWidth = 420;
    inline constexpr int kDesignFilterLabCenterColumnWidth = 524;
    inline constexpr int kDesignFilterLabRightColumnWidth = 280;
    inline constexpr int kDesignFilterLabHeaderHeight = 56;
    inline constexpr int kDesignFilterLabFooterHeight = 40;
    inline constexpr int kDesignFilterLabHeroKnobSize = 64;
    inline constexpr int kDesignFilterLabSecondaryKnobSize = 52;
    inline constexpr int kDesignFilterLabF2HeroKnobSize = 56;
    inline constexpr int kDesignFilterLabRoutingHeroHeight = 120;
    inline constexpr int kDesignFilterLabModRouteRowHeight = 36;

    inline constexpr int kDesignDynamicsLabPageOuterMargin = 16;
    inline constexpr int kDesignDynamicsLabPageSectionGap = 12;
    inline constexpr int kDesignDynamicsLabHeaderHeight = 56;
    inline constexpr int kDesignDynamicsLabFooterHeight = 40;
    inline constexpr int kDesignDynamicsLabKnobSize = 52;
    inline constexpr int kDesignDynamicsLabSignalHeight = 72;

    inline constexpr int kDesignGenerativeLabPageOuterMargin = 16;
    inline constexpr int kDesignGenerativeLabPageSectionGap = 12;
    inline constexpr int kDesignGenerativeLabHeaderHeight = 56;
    inline constexpr int kDesignGenerativeLabFooterHeight = 40;
    inline constexpr int kDesignGenerativeLabKnobSize = 52;

    inline constexpr int kDesignUtilityPeaksPageOuterMargin = 16;
    inline constexpr int kDesignUtilityPeaksPageSectionGap = 12;
    inline constexpr int kDesignUtilityPeaksHeaderHeight = 56;
    inline constexpr int kDesignUtilityPeaksFooterHeight = 32;
    inline constexpr int kDesignUtilityPeaksKnobSize = 48;
    inline constexpr int kDesignUtilityPeaksKnobRowHeight = 84;
    inline constexpr int kDesignUtilityPeaksVizHeight = 56;

    inline constexpr int kDesignEnvelopeSegmentsPageOuterMargin = 16;
    inline constexpr int kDesignEnvelopeSegmentsHeaderHeight = 56;
    inline constexpr int kDesignEnvelopeSegmentsFooterHeight = 40;
    inline constexpr int kDesignEnvelopeSegmentsChainMinHeight = 260;
    inline constexpr int kPlayFocusMorphHubCardHeight = 18;

    /// Figma `murmur-dual-lfo-lab` (15:247).
    inline constexpr int kDesignDualLfoPageOuterMargin = 16;
    inline constexpr int kDesignDualLfoPageSectionGap = 12;
    inline constexpr int kDesignDualLfoLabHeaderHeight = 60;
    inline constexpr int kDesignDualLfoColumnWidth = 419;
    inline constexpr int kDesignDualLfoColumnGap = 12;
    inline constexpr int kDesignDualLfoScopeHeight = 180;
    inline constexpr int kDesignDualLfoKnobsRowHeight = 64;
    inline constexpr int kDesignDualLfoSyncRowHeight = 24;
    inline constexpr int kDesignDualLfoRoutingFooterHeight = 28;
    inline constexpr int kDesignDualLfoBottomBarHeight = 56;
    inline constexpr int kDesignDualLfoQuadColumnCount = 4;
    inline constexpr int kDesignDualLfoFooterChipCount = 8;

    /// Figma `murmur-engine-deep-editor` (28:4) — 1440×1024 reference; letterboxed into 1280×720 embed.
    inline constexpr int kEngineDeepEditorFrameWidth = 1440;
    inline constexpr int kEngineDeepEditorFrameHeight = 1024;
    inline constexpr int kEngineDeepEditorOuterMargin = 24;
    inline constexpr int kEngineDeepEditorHeaderHeight = 64;
    inline constexpr int kEngineDeepEditorBottomBarHeight = 48;
    inline constexpr int kEngineDeepEditorMainContentHeight = 832;
    inline constexpr int kEngineDeepEditorColumnGap = 16;
    inline constexpr int kEngineDeepEditorColumnWidth = 453;
    inline constexpr int kEngineDeepEditorEngineTabHeight = 28;
    inline constexpr int kEngineDeepEditorEngineTabGap = 4;
    inline constexpr int kEngineDeepEditorStateButtonHeight = 19;
    inline constexpr int kEngineDeepEditorFilterColumnHeight = 638;
    inline constexpr int kEngineDeepEditorOscColumnHeight = 625;
    inline constexpr int kEngineDeepEditorAmpColumnHeight = 602;
    inline constexpr int kEngineDeepEditorWaveformDisplayHeight = 200;
    inline constexpr int kEngineDeepEditorFooterPresetBadgeHeight = 19;

    /// Figma `murmur-preset-explorer-overlay` (74:959) — centered modal popup.
    inline constexpr int kPresetExplorerModalWidth = 1040;
    inline constexpr int kPresetExplorerModalHeight = 620;
    inline constexpr int kPresetExplorerTopBarHeight = 44;
    inline constexpr int kPresetExplorerBottomBarHeight = 28;
    inline constexpr int kPresetExplorerLeftColumnWidth = 200;
    inline constexpr int kPresetExplorerCenterColumnWidth = 520;
    inline constexpr int kPresetExplorerRightColumnWidth = 280;
    inline constexpr int kPresetExplorerBankPillHeight = 22;
    inline constexpr int kPresetExplorerCategoryRowHeight = 22;
    inline constexpr int kPresetExplorerPresetRowHeight = 26;
    inline constexpr int kPresetExplorerTableHeaderHeight = 16;
    inline constexpr int kPresetExplorerDetailWaveformHeight = 90;
    inline constexpr int kPresetExplorerLoadButtonHeight = 28;
    inline constexpr int kPresetExplorerSecondaryButtonHeight = 22;
    inline constexpr int kPresetExplorerSearchBarWidth = 320;
    inline constexpr int kPresetExplorerSearchBarHeight = 20;
    inline constexpr int kPresetExplorerFacetRowHeight = 22;
    inline constexpr int kPresetExplorerFacetRowGap = 4;
    inline constexpr int kPresetExplorerFacetStripHeight =
        kPresetExplorerFacetRowHeight * 3 + kPresetExplorerFacetRowGap * 2;

    /// Figma `murmur-preset-browser` (27:6) — full-page 240/696/300 layout.
    inline constexpr int kPresetBrowserPageOuterMargin = 12;
    inline constexpr int kPresetBrowserPageMainWorkspaceHeight = 576;
    inline constexpr int kPresetBrowserPageColumnGap = 10;
    inline constexpr int kPresetBrowserLeftColumnWidth = 240;
    inline constexpr int kPresetBrowserCenterColumnWidth = 696;
    inline constexpr int kPresetBrowserRightColumnWidth = 300;
    inline constexpr int kPresetBrowserFooterHeight = 40;
    inline constexpr int kPresetBrowserSearchBarWidth = 400;
    inline constexpr int kPresetBrowserSearchFieldHeight = 25;
    inline constexpr int kPresetBrowserCategoryRowHeight = 25;
    inline constexpr int kPresetBrowserPresetRowHeight = 28;
    inline constexpr int kPresetBrowserTableHeaderHeight = 18;
    inline constexpr int kPresetBrowserDetailWaveformHeight = 100;
    inline constexpr int kPresetBrowserLoadButtonHeight = 33;
    inline constexpr int kPresetBrowserActionButtonHeight = 28;
    inline constexpr int kPresetBrowserGridCardWidth = 160;
    inline constexpr int kPresetBrowserGridCardHeight = 88;
    inline constexpr int kPresetBrowserGridGap = 8;
    inline constexpr int kPresetBrowserGridColumns = 4;

} // namespace pw8::plugin::ui::layout
