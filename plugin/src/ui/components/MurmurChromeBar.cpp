#include "MurmurChromeBar.h"

#include <cmath>
#include <optional>

#include "../theme/BrandingAssets.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        static constexpr int kVisibleDesignSectionCount = 10;

        static constexpr const char* kDesignSectionLabels[] = {
            "ENGINE", "ARP", "VOC", "FX", "MOD", "FILTER", "DYN", "GEN", "PEAKS", "SEG",
        };

        static constexpr layout::DesignSubPage kDesignSectionPages[] = {
            layout::DesignSubPage::Engine,
            layout::DesignSubPage::Arp,
            layout::DesignSubPage::Vocoder,
            layout::DesignSubPage::Fx,
            layout::DesignSubPage::ModMatrix,
            layout::DesignSubPage::FilterLab,
            layout::DesignSubPage::DynamicsLab,
            layout::DesignSubPage::GenerativeLab,
            layout::DesignSubPage::UtilityPeaks,
            layout::DesignSubPage::EnvelopeSegments,
        };

        [[nodiscard]] juce::String truncateMetaLine(juce::String meta, int maxChars)
        {
            if (meta.length() <= maxChars)
                return meta;
            return meta.substring(0, juce::jmax(0, maxChars - 1)).trimEnd() + juce::String::charToString(0x2026);
        }

        static constexpr int kInlineLedSize = 5;
        static constexpr int kInlineLedLabelGap = 3;
        static constexpr float kNavLabelFontSize = 7.0f;
        static constexpr float kNavGlowExpand = 2.0f;

        [[nodiscard]] int measureInlineNavItemWidth(const juce::String& label,
                                                    float fontSize = kNavLabelFontSize)
        {
            const auto font = fonts::label(fontSize);
            return kInlineLedSize + kInlineLedLabelGap
                   + static_cast<int>(std::ceil(font.getStringWidthFloat(label)));
        }

        void paintInlineLedNavItem(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label,
                                   bool active)
        {
            juce::Graphics::ScopedSaveState clipScope(g);
            g.reduceClipRegion(bounds);

            auto ledBounds = bounds.withSizeKeepingCentre(kInlineLedSize, kInlineLedSize);
            ledBounds.setX(bounds.getX());
            const auto ledFloat = ledBounds.toFloat();

            if (active)
            {
                g.setColour(palette::kFigmaTeal.withAlpha(0.35f));
                g.fillEllipse(ledFloat.expanded(kNavGlowExpand));
            }
            g.setColour(active ? palette::kFigmaTeal : palette::kFigmaTextDim.withAlpha(0.55f));
            g.fillEllipse(ledFloat);

            auto labelBounds = bounds.withTrimmedLeft(kInlineLedSize + kInlineLedLabelGap);
            g.setFont(fonts::label(kNavLabelFontSize));
            g.setColour(active ? palette::kFigmaTeal : palette::kFigmaTextDim);
            g.drawText(label, labelBounds, juce::Justification::centredLeft, false);
        }

        void paintNavDivider(juce::Graphics& g, juce::Rectangle<int> bounds)
        {
            if (bounds.isEmpty())
                return;

            g.setColour(palette::kBorder.withAlpha(0.85f));
            g.fillRect(bounds);
        }
    } // namespace

    MurmurChromeBar::MurmurChromeBar(MurmurProcessor& processor) : processor_(processor)
    {
        presetNameLabel_.setFont(fonts::title(fonts::kPresetNameSize));
        presetNameLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextPrimary);
        presetNameLabel_.setJustificationType(juce::Justification::centredRight);
        presetNameLabel_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(presetNameLabel_);

        presetMetaLabel_.setFont(fonts::label(8.0f));
        presetMetaLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextDim);
        presetMetaLabel_.setJustificationType(juce::Justification::centredRight);
        presetMetaLabel_.setMinimumHorizontalScale(0.7f);
        presetMetaLabel_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(presetMetaLabel_);

        browseButton_.setVisible(false);
        browseButton_.onClick = [this] {
            if (onBrowseRequested)
                onBrowseRequested();
        };
        addAndMakeVisible(browseButton_);

        playViewToggleButton_.setVisible(false);
        playViewToggleButton_.onClick = [this] {
            if (onEditorModeChanged)
                onEditorModeChanged(layout::EditorMode::Design);
        };
        addChildComponent(playViewToggleButton_);

        bpmLabel_.setFont(fonts::label(10.0f));
        bpmLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextPrimary);
        bpmLabel_.setJustificationType(juce::Justification::centredLeft);
        bpmLabel_.setInterceptsMouseClicks(false, false);
        addChildComponent(bpmLabel_);

        masterOutLabel_.setFont(fonts::label(8.0f));
        masterOutLabel_.setColour(juce::Label::textColourId, palette::kFigmaTeal);
        masterOutLabel_.setJustificationType(juce::Justification::centredRight);
        masterOutLabel_.setText("MASTER", juce::dontSendNotification);
        masterOutLabel_.setInterceptsMouseClicks(false, false);
        addChildComponent(masterOutLabel_);

        masterVolumeKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "MASTER");
        masterVolumeKnob_->applyFigmaContext(figma::KnobContext::ChromeMaster);
        addAndMakeVisible(*masterVolumeKnob_);

        startTimerHz(8);
    }

    MurmurChromeBar::~MurmurChromeBar() { stopTimer(); }

    bool MurmurChromeBar::isCompactChrome() const noexcept
    {
        return compactWindow_ || playViewMode_ == layout::PlayViewMode::Compact;
    }

    bool MurmurChromeBar::isDesignTwoRow() const noexcept
    {
        return editorMode_ == layout::EditorMode::Design && !isCompactChrome();
    }

    bool MurmurChromeBar::isPlaySubNavVisible() const noexcept
    {
        return editorMode_ == layout::EditorMode::Play && !isCompactChrome();
    }

    MurmurChromeBar::ViewTab MurmurChromeBar::activeViewTab() const noexcept
    {
        if (editorMode_ == layout::EditorMode::Design)
            return ViewTab::Design;
        if (playViewMode_ == layout::PlayViewMode::Compact)
            return ViewTab::Compact;
        return ViewTab::Play;
    }

    void MurmurChromeBar::setEditorMode(layout::EditorMode mode)
    {
        editorMode_ = mode;
        updatePlayViewToggleButton();
        repaint();
        resized();
    }

    void MurmurChromeBar::setPlayViewMode(layout::PlayViewMode mode)
    {
        playViewMode_ = mode;
        updatePlayViewToggleButton();
        repaint();
        resized();
    }

    void MurmurChromeBar::setDesignSubPage(layout::DesignSubPage page)
    {
        designSubPage_ = page;
        repaint();
    }

    void MurmurChromeBar::setCompactWindowMode(bool compactWindow)
    {
        compactWindow_ = compactWindow;
        updatePlayViewToggleButton();
        repaint();
        resized();
    }

    void MurmurChromeBar::updatePlayViewToggleButton()
    {
        playViewToggleButton_.setVisible(false);
    }

    void MurmurChromeBar::setBrowseFilters(const content::PresetMetadataFilter& filter)
    {
        browseFilter_ = filter;
        refreshPresetDisplay();
    }

    void MurmurChromeBar::refreshPresetDisplay()
    {
        timerCallback();
    }

    void MurmurChromeBar::timerCallback()
    {
        if (!isCompactChrome())
        {
            const auto& patch = processor_.getCurrentPatch();
            const juce::String name = patch.metadata.name.empty() ? juce::String("INIT PATCH")
                                                                  : juce::String(patch.metadata.name);
            juce::String displayName = name.toUpperCase();
            if (processor_.isPatchDirty())
                displayName += " *";
            presetNameLabel_.setText(displayName, juce::dontSendNotification);
            presetNameLabel_.setFont(fonts::title(fonts::kPresetNameSize));
            presetNameLabel_.setColour(juce::Label::textColourId,
                                       processor_.isPatchDirty() ? palette::kAccentWarm : palette::kFigmaTextPrimary);

            juce::String meta;
            if (const auto path = processor_.getCurrentPresetPath(); path.isNotEmpty())
                meta = juce::File(path).getParentDirectory().getFileName().toUpperCase();
            else
                meta = "FACTORY";

            if (presetIndex_ != nullptr)
            {
                const juce::StringArray* favoritesOnly =
                    browseFilter_.favoritesOnly && favoritesStore_ != nullptr ? &favoritesStore_->paths() : nullptr;
                const int total = presetIndex_->filtered(browseFilter_, favoritesOnly).size();
                if (browseFilter_.favoritesOnly)
                    meta = "FAVORITES" + juce::String(fonts::kSep) + juce::String(total) + " presets";
                else if (browseFilter_.category.isNotEmpty())
                    meta = browseFilter_.category.toUpperCase() + juce::String(fonts::kSep) + juce::String(total)
                           + " presets";
                else if (browseFilter_.isNarrowed())
                    meta = "FILTERED" + juce::String(fonts::kSep) + juce::String(total) + " presets";
                else
                    meta += juce::String(fonts::kSep) + juce::String(total) + " presets";

                if (browseFilter_.mood.isNotEmpty())
                    meta += juce::String(fonts::kSep) + browseFilter_.mood.toUpperCase();
                if (browseFilter_.genre.isNotEmpty())
                    meta += juce::String(fonts::kSep) + browseFilter_.genre.toUpperCase();
                if (browseFilter_.tag.isNotEmpty())
                    meta += juce::String(fonts::kSep) + "#" + browseFilter_.tag.toUpperCase();
            }

            presetMetaLabel_.setText(truncateMetaLine(meta, 48), juce::dontSendNotification);
        }

        if (processor_.consumeMorphKeyframeCrossFlash())
        {
            frStepFlashTicks_ = 3; // ~375ms at 8Hz — Figma `89:1763` 300ms fade spec
            const int crossed = processor_.getMorphKeyframeCrossedIndex();
            const auto& mk = processor_.getCurrentPatch().morphKoin;
            if (crossed >= 0 && static_cast<std::size_t>(crossed) < mk.keyframes.size())
                frStepKeyframeName_ = juce::String(mk.keyframes[static_cast<std::size_t>(crossed)].name.c_str());
            else
                frStepKeyframeName_.clear();
        }
        else if (frStepFlashTicks_ > 0)
            --frStepFlashTicks_;

        if (editorMode_ == layout::EditorMode::Play && !isCompactChrome())
        {
            const float bpm = processor_.getHostBpm();
            const juce::String playState = processor_.getHostIsPlaying() ? "PLAY" : "STOP";
            bpmLabel_.setText(juce::String(bpm, 1) + " BPM" + juce::String(fonts::kSep) + "HOST" + juce::String(fonts::kSep) + playState,
                              juce::dontSendNotification);
        }
    }

    void MurmurChromeBar::stepPreset(int direction)
    {
        if (isCompactChrome())
            return;

        if (presetIndex_ == nullptr)
            return;

        const auto current = processor_.getCurrentPresetPath();
        const juce::StringArray* favoritesOnly =
            browseFilter_.favoritesOnly && favoritesStore_ != nullptr ? &favoritesStore_->paths() : nullptr;
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_->nextAfter(current, browseFilter_, favoritesOnly);
        else
            entry = presetIndex_->prevBefore(current, browseFilter_, favoritesOnly);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
    }

    void MurmurChromeBar::layoutViewTabBounds()
    {
        const bool compactChrome = isCompactChrome();
        const bool designTwoRow = isDesignTwoRow();
        const bool playSubNav = isPlaySubNavVisible();
        viewTabBounds_.fill({});
        viewSectionDividerBounds_ = {};
        scoreLineBounds_ = {};

        if (compactChrome)
        {
            constexpr int kCompactExitPlayWidth = 40;
            const int tabY = 10;
            const int tabHeight = layout::kChromeBarSubNavHeight;
            const int tabX = getWidth() - layout::kChromeBarPaddingX - kCompactExitPlayWidth;
            viewTabBounds_[static_cast<std::size_t>(ViewTab::Play)] = {tabX, tabY, kCompactExitPlayWidth, tabHeight};
            return;
        }

        static constexpr const char* kDesktopLabels[] = {"COMPACT", "PLAY", "DESIGN"};

        const int viewTabGap = layout::kChromeViewLedGap;
        const int tabHeight = layout::kChromeBarSubNavHeight;

        int tabX = navContentStartX();

        const int topRowHeight =
            designTwoRow ? layout::kChromeBarDesignTopRowHeight : layout::kChromeBarTopRowHeight;
        const int tabY = layout::kChromeBarPaddingY + (topRowHeight - tabHeight) / 2;

        tabX = juce::jmax(tabX, layout::kChromeBarPaddingX);
        for (int i = 0; i < 3; ++i)
        {
            const int width = measureInlineNavItemWidth(kDesktopLabels[i]);
            viewTabBounds_[static_cast<std::size_t>(i)] = {tabX, tabY, width, tabHeight};
            tabX += width + viewTabGap;
        }

        const int scoreHeight =
            designTwoRow ? juce::jmin(layout::kChromeBarScoreLineHeight, layout::kChromeBarDesignTopRowHeight - 4)
                         : layout::kChromeBarScoreLineHeight;
        const int scoreY = layout::kChromeBarPaddingY + (topRowHeight - scoreHeight) / 2;

        if (playSubNav)
        {
            tabX += layout::kChromeNavSectionGap - viewTabGap;
            viewSectionDividerBounds_ = {tabX, scoreY, layout::kChromeBarScoreLineWidth, scoreHeight};
            return;
        }

        const int scoreX = viewTabBounds_[2].getRight() + layout::kChromeNavSectionGap;
        scoreLineBounds_ = {scoreX, scoreY, layout::kChromeBarScoreLineWidth, scoreHeight};
    }

    void MurmurChromeBar::layoutPlaySubNavBounds()
    {
        playSubNavBounds_.fill({});
        if (!isPlaySubNavVisible() || viewTabBounds_[2].isEmpty())
            return;

        static constexpr const char* kPlaySubNavLabels[] = {"DESKTOP", "BOARD"};

        const int tabY = layout::kChromeBarPaddingY
                         + (layout::kChromeBarTopRowHeight - layout::kChromeBarSubNavHeight) / 2;
        int subX = viewSectionDividerBounds_.isEmpty()
                       ? viewTabBounds_[2].getRight() + layout::kChromeNavSectionGap
                       : viewSectionDividerBounds_.getRight() + layout::kChromeBarSubNavGap;

        for (int i = 0; i < 2; ++i)
        {
            const int width = measureInlineNavItemWidth(kPlaySubNavLabels[i]);
            playSubNavBounds_[static_cast<std::size_t>(i)] = {subX, tabY, width, layout::kChromeBarSubNavHeight};
            subX += width + layout::kChromeBarSubNavTabGap;
        }

        const int scoreHeight = layout::kChromeBarScoreLineHeight;
        const int scoreY = layout::kChromeBarPaddingY + (layout::kChromeBarTopRowHeight - scoreHeight) / 2;
        const int scoreX = playSubNavBounds_[1].getRight() + layout::kChromeNavSectionGap;
        scoreLineBounds_ = {scoreX, scoreY, layout::kChromeBarScoreLineWidth, scoreHeight};
    }

    void MurmurChromeBar::layoutDesignSubNavBounds()
    {
        designSubNavBounds_.fill({});
        if (!isDesignTwoRow())
            return;

        const int tabY = layout::kChromeBarPaddingY + layout::kChromeBarDesignTopRowHeight
                         + layout::kChromeBarDesignSubNavGap;
        const int tabHeight = layout::kChromeBarDesignSubNavRowHeight;
        int tabX = navContentStartX();

        for (int i = 0; i < kVisibleDesignSectionCount; ++i)
        {
            const int width = measureInlineNavItemWidth(kDesignSectionLabels[i]);
            designSubNavBounds_[static_cast<std::size_t>(i)] = {tabX, tabY, width, tabHeight};
            tabX += width + layout::kChromeSectionLedGap;
        }
    }

    int MurmurChromeBar::viewTabIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < static_cast<int>(viewTabBounds_.size()); ++i)
        {
            if (viewTabBounds_[static_cast<std::size_t>(i)].expanded(4, 6).contains(pos))
                return i;
        }
        return -1;
    }

    int MurmurChromeBar::designSubNavIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < kVisibleDesignSectionCount; ++i)
        {
            if (designSubNavBounds_[static_cast<std::size_t>(i)].expanded(2, 4).contains(pos))
                return i;
        }
        return -1;
    }

    int MurmurChromeBar::playSubNavIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < static_cast<int>(playSubNavBounds_.size()); ++i)
        {
            if (playSubNavBounds_[static_cast<std::size_t>(i)].expanded(2, 4).contains(pos))
                return i;
        }
        return -1;
    }

    bool MurmurChromeBar::presetChevronAt(juce::Point<int> pos, int& direction) const
    {
        if (presetPrevBounds_.contains(pos))
        {
            direction = -1;
            return true;
        }
        if (presetNextBounds_.contains(pos))
        {
            direction = 1;
            return true;
        }
        return false;
    }

    void MurmurChromeBar::mouseDown(const juce::MouseEvent& event)
    {
        if (isCompactChrome())
        {
            const int viewTab = viewTabIndexAt(event.getPosition());
            if (viewTab == static_cast<int>(ViewTab::Play))
            {
                if (onEditorModeChanged)
                    onEditorModeChanged(layout::EditorMode::Play);
                if (onPlayViewModeChanged)
                    onPlayViewModeChanged(layout::PlayViewMode::Desktop);
            }
            return;
        }

        if (savePatchButtonAt(event.getPosition()))
        {
            promptSavePatch();
            return;
        }

        int presetDir = 0;
        if (presetChevronAt(event.getPosition(), presetDir))
        {
            stepPreset(presetDir);
            return;
        }

        if (presetDisplayBounds_.contains(event.getPosition()))
        {
            if (onBrowseRequested)
                onBrowseRequested();
            return;
        }

        if (isPlaySubNavVisible())
        {
            const int playSub = playSubNavIndexAt(event.getPosition());
            if (playSub == 0)
            {
                if (onPlayViewModeChanged)
                    onPlayViewModeChanged(layout::PlayViewMode::Desktop);
                return;
            }
            if (playSub == 1)
            {
                if (onEditorModeChanged)
                    onEditorModeChanged(layout::EditorMode::Design);
                if (onDesignSubPageChanged)
                    onDesignSubPageChanged(layout::DesignSubPage::Engine);
                return;
            }
        }

        const int viewTab = viewTabIndexAt(event.getPosition());
        if (viewTab >= 0)
        {
            const auto active = activeViewTab();
            if (viewTab == static_cast<int>(active))
            {
                if (viewTab == static_cast<int>(ViewTab::Compact))
                {
                    if (onEditorModeChanged)
                        onEditorModeChanged(layout::EditorMode::Play);
                    if (onPlayViewModeChanged)
                        onPlayViewModeChanged(layout::PlayViewMode::Compact);
                }
                return;
            }

            if (viewTab == static_cast<int>(ViewTab::Compact))
            {
                if (onEditorModeChanged)
                    onEditorModeChanged(layout::EditorMode::Play);
                if (onPlayViewModeChanged)
                    onPlayViewModeChanged(layout::PlayViewMode::Compact);
            }
            else if (viewTab == static_cast<int>(ViewTab::Play))
            {
                if (onEditorModeChanged)
                    onEditorModeChanged(layout::EditorMode::Play);
                if (onPlayViewModeChanged)
                    onPlayViewModeChanged(layout::PlayViewMode::Desktop);
            }
            else if (viewTab == static_cast<int>(ViewTab::Design))
            {
                if (onEditorModeChanged)
                    onEditorModeChanged(layout::EditorMode::Design);
                if (onDesignSubPageChanged)
                    onDesignSubPageChanged(layout::DesignSubPage::Engine);
            }
            return;
        }

        const int subNav = designSubNavIndexAt(event.getPosition());
        if (subNav < 0 || editorMode_ != layout::EditorMode::Design)
            return;

        const auto page = kDesignSectionPages[static_cast<std::size_t>(subNav)];
        if (page == designSubPage_)
            return;

        if (onDesignSubPageChanged)
            onDesignSubPageChanged(page);
    }

    void MurmurChromeBar::mouseDoubleClick(const juce::MouseEvent& event)
    {
        if (editorMode_ != layout::EditorMode::Play || playViewMode_ == layout::PlayViewMode::Compact)
            return;

        auto brandArea = getLocalBounds().reduced(layout::kChromeBarPaddingX, layout::kChromeBarTopRowPaddingY);
        brandArea = brandArea.removeFromLeft(layout::kChromeBarBrandWidth);
        if (!brandArea.contains(event.getPosition()))
            return;

        if (onEditorModeChanged)
            onEditorModeChanged(layout::EditorMode::Design);
    }

    void MurmurChromeBar::paintBrand(juce::Graphics& g, juce::Rectangle<int> area) const
    {
        if (isCompactChrome() || area.isEmpty())
            return;

        juce::Graphics::ScopedSaveState clipScope(g);
        g.reduceClipRegion(area);

        const int inset = layout::kChromeBarLogoInset;
        auto inner = area.reduced(0, inset);
        if (inner.isEmpty())
            return;

        const int maxLogoHeight = isDesignTwoRow() ? layout::kChromeBarDesignLogoMaxHeight
                                                   : layout::kChromeBarPlayLogoMaxHeight;
        const float maxW = static_cast<float>(inner.getWidth());
        const float maxH = static_cast<float>(juce::jmin(maxLogoHeight, inner.getHeight()));

        float logoW = static_cast<float>(branding::logoLockupWidth());
        float logoH = static_cast<float>(branding::logoLockupHeight());
        const float scale = juce::jmin(maxW / logoW, maxH / logoH);
        logoW *= scale;
        logoH *= scale;

        auto logoBounds =
            juce::Rectangle<float>(static_cast<float>(inner.getX()),
                                   static_cast<float>(inner.getCentreY()) - logoH * 0.5f, logoW, logoH);
        branding::paintLogoLockup(g, logoBounds);
    }

    void MurmurChromeBar::paintCompactLogo(juce::Graphics& g) const
    {
        if (!isCompactChrome())
            return;

        auto header = getLocalBounds().reduced(10, 6);
        const float logoW = static_cast<float>(branding::compactLogoWidth());
        const float logoH = static_cast<float>(branding::compactLogoHeight());
        auto logoBounds = juce::Rectangle<float>(header.getRight() - logoW, header.getCentreY() - logoH * 0.5f, logoW,
                                                 logoH);
        branding::paintLogoLockup(g, logoBounds);
    }

    void MurmurChromeBar::paintViewTabs(juce::Graphics& g)
    {
        const auto active = activeViewTab();

        if (isCompactChrome())
        {
            const auto tab = viewTabBounds_[static_cast<std::size_t>(ViewTab::Play)];
            if (!tab.isEmpty())
                paintInlineLedNavItem(g, tab, "PLAY", false);
            return;
        }

        static constexpr const char* kDesktopLabels[] = {"COMPACT", "PLAY", "DESIGN"};

        for (int i = 0; i < 3; ++i)
        {
            const auto tab = viewTabBounds_[static_cast<std::size_t>(i)];
            if (tab.isEmpty())
                continue;

            const juce::String label = kDesktopLabels[i];
            const bool tabActive = static_cast<ViewTab>(i) == active;
            paintInlineLedNavItem(g, tab, label, tabActive);
        }
    }

    void MurmurChromeBar::paintViewSectionDivider(juce::Graphics& g) const
    {
        paintNavDivider(g, viewSectionDividerBounds_);
    }

    void MurmurChromeBar::paintPlaySubNav(juce::Graphics& g)
    {
        if (!isPlaySubNavVisible())
            return;

        static constexpr const char* kPlaySubNavLabels[] = {"DESKTOP", "BOARD"};
        for (int i = 0; i < 2; ++i)
        {
            const auto tab = playSubNavBounds_[static_cast<std::size_t>(i)];
            if (tab.isEmpty())
                continue;

            const bool active =
                i == 0 && layout::isDesktopPlayLayout(playViewMode_) && playViewMode_ != layout::PlayViewMode::Compact;
            paintInlineLedNavItem(g, tab, kPlaySubNavLabels[i], active);
        }
    }

    void MurmurChromeBar::paintDesignSubNav(juce::Graphics& g)
    {
        if (!isDesignTwoRow())
            return;

        for (int i = 0; i < kVisibleDesignSectionCount; ++i)
        {
            const auto tab = designSubNavBounds_[static_cast<std::size_t>(i)];
            if (tab.isEmpty())
                continue;

            const bool active = designSubPage_ == kDesignSectionPages[static_cast<std::size_t>(i)];
            paintInlineLedNavItem(g, tab, kDesignSectionLabels[i], active);
        }
    }

    void MurmurChromeBar::paintScoreLine(juce::Graphics& g) const
    {
        if (isCompactChrome() || scoreLineBounds_.isEmpty())
            return;

        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.fillRect(scoreLineBounds_);

        if (frStepFlashTicks_ > 0)
        {
            const float alpha = juce::jlimit(0.35f, 1.0f, static_cast<float>(frStepFlashTicks_) / 3.0f);
            auto badge = scoreLineBounds_.toFloat().translated(static_cast<float>(scoreLineBounds_.getWidth()) + 6.0f,
                                                               -2.0f);
            badge.setWidth(frStepKeyframeName_.isEmpty() ? 52.0f : 96.0f);
            badge.setHeight(14.0f);
            g.setColour(palette::kFigmaTeal.withAlpha(alpha * 0.25f));
            g.fillRoundedRectangle(badge, 3.0f);
            g.setColour(palette::kFigmaTeal.withAlpha(alpha));
            g.drawRoundedRectangle(badge.reduced(0.5f), 3.0f, 1.0f);
            g.setFont(fonts::label(7.0f));
            const juce::String label =
                frStepKeyframeName_.isEmpty() ? "FR.STEP" : ("FR.STEP · " + frStepKeyframeName_.toUpperCase());
            g.drawText(label, badge, juce::Justification::centred, true);
        }
    }

    void MurmurChromeBar::paintPresetBrowserChrome(juce::Graphics& g) const
    {
        if (isCompactChrome() || presetDisplayBounds_.isEmpty())
            return;

        auto pill = presetDisplayBounds_.toFloat().expanded(6.0f, 4.0f);
        if (!presetPrevBounds_.isEmpty())
            pill = pill.getUnion(presetPrevBounds_.toFloat().expanded(2.0f, 4.0f));
        if (!presetNextBounds_.isEmpty())
            pill = pill.getUnion(presetNextBounds_.toFloat().expanded(2.0f, 4.0f));

        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(pill, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(pill.reduced(0.75f), 8.0f, 1.0f);

        if (!presetPrevBounds_.isEmpty())
        {
            g.setColour(palette::kFigmaTextDim);
            g.setFont(fonts::label(9.0f));
            g.drawText("<", presetPrevBounds_.toFloat(), juce::Justification::centred, true);
        }
        if (!presetNextBounds_.isEmpty())
        {
            g.setColour(palette::kFigmaTextDim);
            g.setFont(fonts::label(9.0f));
            g.drawText(">", presetNextBounds_.toFloat(), juce::Justification::centred, true);
        }

        paintSavePatchButton(g);
    }

    void MurmurChromeBar::paintSavePatchButton(juce::Graphics& g) const
    {
        if (savePatchButtonBounds_.isEmpty() || !processor_.isPatchDirty())
            return;

        auto b = savePatchButtonBounds_.toFloat().reduced(2.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.18f));
        g.fillRoundedRectangle(b, 4.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.95f));
        g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);

        auto icon = b.reduced(3.0f);
        g.fillRect(icon.removeFromLeft(icon.getWidth() * 0.42f));
        g.fillRect(icon.withHeight(icon.getHeight() * 0.55f));
    }

    bool MurmurChromeBar::savePatchButtonAt(juce::Point<int> pos) const
    {
        return processor_.isPatchDirty() && savePatchButtonBounds_.contains(pos);
    }

    void MurmurChromeBar::promptSavePatch()
    {
        if (isCompactChrome() || !processor_.isPatchDirty())
            return;

        const auto path = processor_.getCurrentPresetPath();
        const bool canOverwrite = processor_.isUserPresetPath(path);

        juce::PopupMenu menu;
        if (canOverwrite)
            menu.addItem(1, "Save (Overwrite)");
        menu.addItem(2, "Save As Copy...");
        menu.showMenuAsync(juce::PopupMenu::Options(),
                           [this, canOverwrite, path](int result) {
                               if (result == 1 && canOverwrite)
                               {
                                   processor_.saveCurrentPatchToFile(path);
                                   refreshPresetDisplay();
                               }
                               else if (result == 2)
                               {
                                   launchSaveAsCopyDialog();
                               }
                           });
    }

    void MurmurChromeBar::launchSaveAsCopyDialog()
    {
        const auto defaultDir = processor_.userPresetsDirectory();
        defaultDir.createDirectory();

        const auto& meta = processor_.getCurrentPatch().metadata;
        juce::String suggested = meta.name.empty() ? juce::String("untitled") : juce::String(meta.name);
        suggested = suggested.replaceCharacter(' ', '-').toLowerCase();
        // New saves always default to the current .murmur extension -- see
        // docs/REBRAND_MURMUR.md. The load-side filter (PatchBrowserBar) still accepts
        // legacy .pw8 files; this is a save/write path, which only ever writes .murmur.
        if (!suggested.endsWithIgnoreCase(".murmur"))
            suggested += ".murmur";

        auto chooser = std::make_shared<juce::FileChooser>("Save patch as copy", defaultDir.getChildFile(suggested),
                                                           "*.murmur");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                             [this, chooser](const juce::FileChooser& fc) {
                                 auto file = fc.getResult();
                                 if (file == juce::File{})
                                     return;
                                 if (!file.hasFileExtension("murmur"))
                                     file = file.withFileExtension("murmur");
                                 if (processor_.saveCurrentPatchToFile(file.getFullPathName()))
                                     refreshPresetDisplay();
                             });
    }

    void MurmurChromeBar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        auto content = getLocalBounds().reduced(layout::kChromeBarPaddingX, layout::kChromeBarPaddingY);
        if (isCompactChrome())
            content = getLocalBounds().reduced(10, 6);

        auto brandArea = content.removeFromLeft(isCompactChrome() ? 80 : layout::kChromeBarBrandWidth);
        if (isDesignTwoRow())
            brandArea.setHeight(layout::kChromeBarDesignTopRowHeight);
        paintBrand(g, brandArea);

        paintViewTabs(g);
        paintViewSectionDivider(g);
        paintPlaySubNav(g);
        paintScoreLine(g);
        paintDesignSubNav(g);
        paintPresetBrowserChrome(g);
        paintCompactLogo(g);
    }

    void MurmurChromeBar::resized()
    {
        layoutViewTabBounds();
        layoutPlaySubNavBounds();
        layoutDesignSubNavBounds();

        auto right = getLocalBounds().reduced(layout::kChromeBarPaddingX, layout::kChromeBarPaddingY);
        if (isCompactChrome())
        {
            right = getLocalBounds().reduced(10, 6);
            presetPrevBounds_ = {};
            presetNextBounds_ = {};
            presetDisplayBounds_ = {};
            savePatchButtonBounds_ = {};
            browseButton_.setVisible(false);
            bpmLabel_.setVisible(false);
            masterOutLabel_.setVisible(false);
            masterVolumeKnob_->setVisible(false);
            presetMetaLabel_.setVisible(false);
            presetNameLabel_.setVisible(false);
            playViewToggleButton_.setVisible(false);
            return;
        }

        const bool designTwoRow = isDesignTwoRow();
        const bool showBrowse = !isCompactChrome();
        browseButton_.setVisible(showBrowse);

        bpmLabel_.setVisible(editorMode_ == layout::EditorMode::Play);
        masterOutLabel_.setVisible(false);
        presetMetaLabel_.setVisible(true);
        masterVolumeKnob_->setVisible(false);

        if (showBrowse)
            right.removeFromRight(8);
        if (showBrowse)
        {
            browseButton_.setBounds(right.removeFromRight(58).withSizeKeepingCentre(58, 22));
            right.removeFromRight(8);
        }

        auto presetBlock = right.removeFromRight(layout::kChromePresetDisplayWidth);
        if (designTwoRow)
            presetBlock.setHeight(layout::kChromeBarDesignTopRowHeight);

        presetDisplayBounds_ = presetBlock;

        presetPrevBounds_ = presetBlock.removeFromLeft(layout::kChromePresetChevronWidth);
        savePatchButtonBounds_ = presetBlock.removeFromLeft(18);
        presetBlock.removeFromLeft(4);

        presetNextBounds_ = presetBlock.removeFromRight(layout::kChromePresetChevronWidth);

        auto metaRow = presetBlock.removeFromBottom(10);
        masterOutLabel_.setBounds(metaRow.removeFromRight(48));
        metaRow.removeFromRight(6);
        presetMetaLabel_.setBounds(metaRow);

        presetNameLabel_.setBounds(presetBlock.removeFromTop(18));
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetBlock);
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetNameLabel_.getBounds());
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetMetaLabel_.getBounds());
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(savePatchButtonBounds_);
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetPrevBounds_);
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetNextBounds_);

        if (editorMode_ == layout::EditorMode::Play && !scoreLineBounds_.isEmpty())
        {
            const int perfY = layout::kChromeBarPaddingY + 5;
            updatePlayViewToggleButton();
            if (playViewToggleButton_.isVisible())
            {
                playViewToggleButton_.setBounds(scoreLineBounds_.getRight() + 10, perfY - 2, 72, 18);
                bpmLabel_.setBounds(playViewToggleButton_.getRight() + 12, perfY, 72, 13);
            }
            else
            {
                bpmLabel_.setBounds(scoreLineBounds_.getRight() + 16, perfY, 72, 13);
            }
        }
        else
        {
            playViewToggleButton_.setVisible(false);
        }
    }

} // namespace pw8::plugin::ui
