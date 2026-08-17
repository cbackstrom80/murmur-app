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
    inline constexpr int kArpStepLaneWidth = 32;
    inline constexpr int kArpStepLaneHeight = 130;
    inline constexpr int kArpStepMetaHeight = 39;
    inline constexpr int kArpStepSequencerRowHeight = 191;
    inline constexpr int kArpSeqHeaderHeight = 13;
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

    /// Compact mode — Figma `murmur-play-compact` (4:1134), fixed 320px width, resizable height.
    inline constexpr int kCompactWidth = 320;
    inline constexpr int kCompactMinHeight = 480;
    inline constexpr int kCompactMaxHeight = 1200;
    inline constexpr int kCompactDefaultHeight = 565;
    inline constexpr int kCompactOuterMargin = 14;
    inline constexpr int kCompactHeaderHeight = 28;
    inline constexpr int kCompactMissionCardHeight = kCompactHeaderHeight + 38;
    inline constexpr int kCompactScopePanelHeight = 152;
    inline constexpr int kCompactScopeSize = 148;
    inline constexpr int kCompactScopePanelPadding = 12;
    inline constexpr int kCompactScopeHeaderHeight = 8;
    inline constexpr int kCompactScopeOscHeight = 110;
    inline constexpr int kCompactScopeHeaderGap = 8;
    inline constexpr int kCompactOutputMeterHeight = 6;
    inline constexpr int kCompactFooterSystemHeight = 30;
    inline constexpr int kCompactModChipHeight = 14;
    inline constexpr int kCompactModChipGap = 4;
    inline constexpr int kCompactMacroPanelPadding = 12;
    inline constexpr int kCompactMacroKnobSize = 36;
    inline constexpr int kCompactMacroKnobGap = 8;
    inline constexpr int kCompactMacroRowGap = 8;
    inline constexpr int kCompactOutputBlockHeight = 72;
    inline constexpr int kCompactMasterKnobSize = 48;
    inline constexpr int kCompactVolumeHeight = kCompactOutputBlockHeight;
    inline constexpr int kCompactVolumeKnobWidth = kCompactMasterKnobSize;
    inline constexpr int kCompactBlockGap = 12;

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
    inline constexpr int kDesktopPlayModeOuterMargin = 20;
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

    /// Figma `ipad-play-view` (4:2472) — Basic PLAY layout adapted for desktop content area.
    inline constexpr int kIpadPlayUpperDeckHeight = 280;
    inline constexpr int kIpadPlayUpperDeckGap = 12;
    inline constexpr int kIpadPlayMasterDeckWidth = 280;
    inline constexpr int kIpadPlayMasterKnobSize = 110;
    inline constexpr int kIpadPlayMacrosDeckHeight = 201;
    inline constexpr int kIpadPlayMacroCount = 6;
    inline constexpr int kIpadPlayMacroKnobWidth = 120;
    inline constexpr int kIpadPlayMacroKnobGap = 24;

    enum class PlayViewMode
    {
        Basic,
        Advanced,
        Compact,
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
    };

    /// Figma unified header-bar — `50:252` compact / `39:142` play / `39:2` design.
    inline constexpr int kChromeBarHeightCompact = 28;
    inline constexpr int kChromeBarHeightPlay = 60;
    inline constexpr int kChromeBarHeightDesign = 60;
    inline constexpr int kChromeBarPaddingX = 16;
    inline constexpr int kChromeBarPaddingY = 10;
    inline constexpr int kChromeBarTopRowPaddingY = 10;
    inline constexpr int kChromeBarTopRowHeight = 40;
    inline constexpr int kChromeBarBrandWidth = 220;
    inline constexpr int kChromeBarScoreLineWidth = 1;
    inline constexpr int kChromeBarScoreLineHeight = 24;
    inline constexpr int kChromeBarLedSize = 6;
    inline constexpr int kChromeBarLedGap = 4;
    inline constexpr int kChromeViewLedGap = 14;
    inline constexpr int kChromeSectionLedGap = 12;
    inline constexpr int kChromeBarSubNavGap = 6;
    inline constexpr int kChromeBarSubNavHeight = 18;
    inline constexpr int kChromeBarSubNavTabGap = 12;
    inline constexpr int kChromeViewTabHeight = 19;
    inline constexpr int kChromeViewTabCompactWidth = 71;
    inline constexpr int kChromeViewTabPlayWidth = 48;
    inline constexpr int kChromeViewTabDesignWidth = 58;
    inline constexpr int kChromeViewTabGap = 4;
    inline constexpr int kChromeViewTabCmpWidth = 30;
    inline constexpr int kChromeViewTabPlyWidth = 17;
    inline constexpr int kChromeViewTabDsnWidth = 26;
    inline constexpr int kChromeViewTabCompactHeight = 7;
    inline constexpr int kChromeViewTabCompactGap = 6;
    inline constexpr int kChromeSubNavEngineWidth = 26;
    inline constexpr int kChromeSubNavArpWidth = 13;
    inline constexpr int kChromeSubNavVocoderWidth = 30;
    inline constexpr int kChromeSubNavFxWidth = 9;
    inline constexpr int kChromeSubNavModMatrixWidth = 42;
    inline constexpr int kChromeSubNavBrowseWidth = 58;
    inline constexpr int kChromeSubNavWavetableWidth = 72;
    inline constexpr int kChromeSubNavDualLfoWidth = 62;
    inline constexpr int kChromePresetChevronWidth = 12;
    inline constexpr int kChromeMasterKnobSize = 28;
    inline constexpr int kChromePresetDisplayWidth = 260;

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
    inline constexpr int kDesignModeV2GridSectionHeight = 560;
    inline constexpr int kDesignModeV2GridRowHeight = 270;
    inline constexpr int kDesignModeV2GridRowGap = 12;
    inline constexpr int kDesignModeV2GridColGap = 12;
    inline constexpr int kDesignModeV2EngineCardWidth = 303;
    inline constexpr int kDesignModeV2EngineCardHeight = 270;
    inline constexpr int kDesignModeV2StatusBarHeight = 40;
    inline constexpr int kDesignModeV2StatusBarPaddingX = 16;
    inline constexpr int kDesignModeV2CardPadding = 12;
    inline constexpr int kDesignModeV2CardRowGap = 10;
    inline constexpr int kDesignModeV2CardHeaderHeight = 14;
    inline constexpr int kDesignModeV2TypeStripHeight = 14;
    inline constexpr int kDesignModeV2SubPickerHeight = 22;
    inline constexpr int kDesignModeV2SubPickerPillHeight = 14;
    inline constexpr int kDesignModeV2SubPickerArrowWidth = 25;
    inline constexpr int kDesignModeV2ContextVisualizerHeight = 80;
    inline constexpr int kDesignModeV2OscillatorPickerHeight =
        kDesignModeV2TypeStripHeight + kDesignModeV2CardRowGap + kDesignModeV2SubPickerHeight + kDesignModeV2CardRowGap
        + kDesignModeV2ContextVisualizerHeight;
    inline constexpr int kDesignModeV2KnobsEnvelopeRowHeight = 40;
    inline constexpr int kDesignModeV2LevelRowHeight = 10;
    inline constexpr int kDesignModeV2EnvelopeWidth = 68;
    inline constexpr int kDesignModeV2EnvelopeHeight = 34;
    inline constexpr int kDesignModeV2EnvelopeYOffset = 6;

    /// Figma `murmur-design-fx` (35:4) — standalone design FX rack page.
    inline constexpr int kDesignFxPageSectionGap = 12;
    inline constexpr int kDesignFxPageSignalChainLabelHeight = 11;
    inline constexpr int kDesignFxPageSignalChainLabelGap = 6;
    inline constexpr int kDesignFxPageSignalChainPipelineHeight = 94;
    inline constexpr int kDesignFxPageSignalChainSectionHeight =
        kDesignFxPageSignalChainLabelGap + kDesignFxPageSignalChainLabelHeight + kDesignFxPageSignalChainLabelGap
        + kDesignFxPageSignalChainPipelineHeight;
    inline constexpr int kDesignFxPageChipWidth = 88;
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
    inline constexpr int kDesignFxPageDetailBodyHeight = 296;
    inline constexpr int kDesignFxPageDetailControlsWidth = 280;
    inline constexpr int kDesignFxPageEqSidebarWidth = 134;
    inline constexpr int kDesignFxPageDetailControlsVizGap = 16;
    inline constexpr int kDesignFxPageDetailKnobGridHeight = 174;
    inline constexpr int kDesignFxPageDetailKnobWidth = 64;
    inline constexpr int kDesignFxPageDetailKnobHeight = 58;
    inline constexpr int kDesignFxPageDetailKnobColGap = 44;
    inline constexpr int kDesignFxPageDetailKnobRowGap = 58;
    inline constexpr int kDesignFxPageDetailModeStripHeight = 30;
    inline constexpr int kDesignFxPageOverviewWidth = 380;
    inline constexpr int kDesignFxPageOverviewHeight = 360;
    inline constexpr int kDesignFxPageOverviewPadding = 12;
    inline constexpr int kDesignFxPageOverviewRowHeight = 24;
    inline constexpr int kDesignFxPageOverviewRowGap = 4;
    inline constexpr int kDesignFxPageRoutingBarHeight = 54;
    inline constexpr int kDesignFxPageRoutingPaddingX = 16;
    inline constexpr int kDesignFxPageSlotCount = 12;
    inline constexpr int kDesignFxPageDetailKnobDialSize = 32;

    /// Figma `murmur-design-mod-matrix` (27:265) — standalone design mod matrix page.
    inline constexpr int kDesignModMatrixPageOuterMargin = 12;
    inline constexpr int kDesignModMatrixPageSectionGap = 10;
    inline constexpr int kDesignModMatrixPageStatusBarHeight = 40;
    inline constexpr int kDesignModMatrixPageLeftWidth = 966;
    inline constexpr int kDesignModMatrixPageSidebarWidth = 280;
    inline constexpr int kDesignModMatrixPageSidebarGap = 10;
    inline constexpr int kDesignModMatrixPageGridCardHeight = 304;
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

    /// Figma `murmur-dual-lfo-lab` (15:247).
    inline constexpr int kDesignDualLfoPageOuterMargin = 16;
    inline constexpr int kDesignDualLfoPageSectionGap = 12;
    inline constexpr int kDesignDualLfoLabHeaderHeight = 48;
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

    /// Legacy full-page preset browser (27:6) — superseded by explorer modal above.
    inline constexpr int kPresetBrowserPageOuterMargin = 12;
    inline constexpr int kPresetBrowserPageMainWorkspaceHeight = 576;
    inline constexpr int kPresetBrowserPageColumnGap = 10;
    inline constexpr int kPresetBrowserLeftColumnWidth = 240;
    inline constexpr int kPresetBrowserCenterColumnWidth = 696;
    inline constexpr int kPresetBrowserRightColumnWidth = 300;
    inline constexpr int kPresetBrowserFooterHeight = 40;
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
