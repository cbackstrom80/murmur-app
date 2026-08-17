#include "MurmurChromeBar.h"

#include <optional>

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        static constexpr int kVisibleDesignSectionCount = 5;

        static constexpr const char* kDesignSectionLabels[] = {
            "ENGINE", "ARP", "VOC", "FX", "MOD",
        };

        static constexpr int kDesignSectionWidths[] = {
            layout::kChromeSubNavEngineWidth,
            layout::kChromeSubNavArpWidth,
            layout::kChromeSubNavVocoderWidth,
            layout::kChromeSubNavFxWidth,
            layout::kChromeSubNavModMatrixWidth,
        };

        void paintLedDot(juce::Graphics& g, juce::Rectangle<float> area, bool active)
        {
            if (active)
            {
                g.setColour(palette::kFigmaTeal.withAlpha(0.35f));
                g.fillEllipse(area.expanded(3.0f));
            }
            g.setColour(active ? palette::kFigmaTeal : palette::kFigmaTextDim.withAlpha(0.55f));
            g.fillEllipse(area);
        }

        void paintInlineLedNavItem(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label,
                                   bool active)
        {
            const int ledSize = 5;
            auto ledBounds = bounds.withSizeKeepingCentre(ledSize, ledSize);
            ledBounds.setX(bounds.getX());
            paintLedDot(g, ledBounds.toFloat(), active);

            auto labelBounds = bounds.withTrimmedLeft(ledSize + 3);
            g.setFont(fonts::label(7.0f));
            g.setColour(active ? palette::kFigmaTeal : palette::kFigmaTextDim);
            g.drawText(label, labelBounds, juce::Justification::centredLeft, true);
        }

        [[nodiscard]] int viewTabWidths(bool compactChrome, int index)
        {
            if (compactChrome)
            {
                static constexpr int kCompactWidths[] = {22, 20, 21};
                return kCompactWidths[index];
            }

            static constexpr int kDesktopWidths[] = {layout::kChromeViewTabCmpWidth, layout::kChromeViewTabPlyWidth,
                                                     layout::kChromeViewTabDsnWidth};
            return kDesktopWidths[index];
        }
        void paintLedNavItem(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label, bool active,
                             bool compactChrome)
        {
            if (compactChrome)
            {
                paintInlineLedNavItem(g, bounds, label, active);
                return;
            }

            const int ledSize = layout::kChromeBarLedSize;
            const float fontSize = 7.0f;
            const int ledGap = layout::kChromeBarLedGap;

            auto ledBounds = bounds.withHeight(ledSize);
            ledBounds = ledBounds.withSizeKeepingCentre(ledSize, ledSize);
            paintLedDot(g, ledBounds.toFloat(), active);

            auto labelBounds = bounds.withTrimmedTop(ledSize + ledGap);
            g.setFont(fonts::label(fontSize));
            g.setColour(active ? palette::kFigmaTeal : palette::kFigmaTextDim);
            g.drawText(label, labelBounds, juce::Justification::centredTop, true);
        }
    } // namespace

    MurmurChromeBar::MurmurChromeBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        presetNameLabel_.setFont(fonts::label(10.0f));
        presetNameLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextPrimary);
        presetNameLabel_.setJustificationType(juce::Justification::centredRight);
        presetNameLabel_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(presetNameLabel_);

        presetMetaLabel_.setFont(fonts::label(8.0f));
        presetMetaLabel_.setColour(juce::Label::textColourId, palette::kFigmaTextDim);
        presetMetaLabel_.setJustificationType(juce::Justification::centredRight);
        presetMetaLabel_.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(presetMetaLabel_);

        browseButton_.setVisible(false);
        browseButton_.onClick = [this] {
            if (onBrowseRequested)
                onBrowseRequested();
        };
        addAndMakeVisible(browseButton_);

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
        masterVolumeKnob_->setHeaderCompactMode(true);
        masterVolumeKnob_->setMaxDialDiameter(layout::kChromeMasterKnobSize);
        addAndMakeVisible(*masterVolumeKnob_);

        startTimerHz(4);
    }

    MurmurChromeBar::~MurmurChromeBar() { stopTimer(); }

    bool MurmurChromeBar::isCompactChrome() const noexcept
    {
        return compactWindow_ || playViewMode_ == layout::PlayViewMode::Compact;
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
        repaint();
        resized();
    }

    void MurmurChromeBar::setPlayViewMode(layout::PlayViewMode mode)
    {
        playViewMode_ = mode;
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
        repaint();
        resized();
    }

    void MurmurChromeBar::refreshPresetDisplay()
    {
        timerCallback();
    }

    void MurmurChromeBar::timerCallback()
    {
        const auto& patch = processor_.getCurrentPatch();
        const juce::String name = patch.metadata.name.empty() ? juce::String("INIT PATCH")
                                                              : juce::String(patch.metadata.name);
        presetNameLabel_.setText(name.toUpperCase(), juce::dontSendNotification);

        juce::String meta;
        if (const auto path = processor_.getCurrentPresetPath(); path.isNotEmpty())
            meta = juce::File(path).getParentDirectory().getFileName().toUpperCase();
        else
            meta = "FACTORY";

        presetMetaLabel_.setText(meta, juce::dontSendNotification);

        if (editorMode_ == layout::EditorMode::Play && !isCompactChrome())
        {
            const float bpm = processor_.getHostBpm();
            bpmLabel_.setText(juce::String(bpm, 2) + " BPM", juce::dontSendNotification);
        }
    }

    void MurmurChromeBar::stepPreset(int direction)
    {
        if (presetIndex_ == nullptr)
            return;

        juce::StringArray favorites;
        if (favoritesStore_ != nullptr)
            favorites = favoritesStore_->paths();

        const auto current = processor_.getCurrentPresetPath();
        const juce::StringArray* favoritesOnly = favorites.isEmpty() ? nullptr : &favorites;
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_->nextAfter(current, {}, favoritesOnly);
        else
            entry = presetIndex_->prevBefore(current, {}, favoritesOnly);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
    }

    void MurmurChromeBar::layoutViewTabBounds()
    {
        const bool compactChrome = isCompactChrome();
        viewTabBounds_.fill({});
        scoreLineBounds_ = {};

        const int gap = compactChrome ? layout::kChromeViewTabCompactGap : layout::kChromeViewLedGap;

        int tabX = 0;
        if (compactChrome)
            tabX = 117;
        else
            tabX = layout::kChromeBarPaddingX + layout::kChromeBarBrandWidth + 16;

        const int tabHeight = compactChrome ? 7 : layout::kChromeViewTabHeight;
        const int tabY = compactChrome ? 10
                                       : layout::kChromeBarPaddingY
                                             + (layout::kChromeBarTopRowHeight - layout::kChromeViewTabHeight) / 2;

        tabX = juce::jmax(tabX, layout::kChromeBarPaddingX);
        for (int i = 0; i < 3; ++i)
        {
            const int width = viewTabWidths(compactChrome, i);
            viewTabBounds_[static_cast<std::size_t>(i)] = {tabX, tabY, width, tabHeight};
            tabX += width + gap;
        }

        if (!compactChrome)
        {
            const int scoreX = viewTabBounds_[2].getRight() + gap;
            const int scoreY =
                layout::kChromeBarPaddingY + (layout::kChromeBarTopRowHeight - layout::kChromeBarScoreLineHeight) / 2;
            scoreLineBounds_ = {scoreX, scoreY, layout::kChromeBarScoreLineWidth, layout::kChromeBarScoreLineHeight};
        }
    }

    void MurmurChromeBar::layoutDesignSubNavBounds()
    {
        designSubNavBounds_.fill({});
        if (editorMode_ != layout::EditorMode::Design || isCompactChrome())
            return;

        int sectionTotalWidth = 0;
        for (int i = 0; i < kVisibleDesignSectionCount; ++i)
            sectionTotalWidth += kDesignSectionWidths[i];
        sectionTotalWidth += layout::kChromeSectionLedGap * (kVisibleDesignSectionCount - 1);

        const int presetLeft = getWidth() - layout::kChromeBarPaddingX - layout::kChromePresetDisplayWidth;
        const int sectionStart = scoreLineBounds_.getRight() + layout::kChromeSectionLedGap;
        const int sectionEnd = presetLeft - 16;
        int tabX = sectionStart + juce::jmax(0, (sectionEnd - sectionStart - sectionTotalWidth) / 2);
        const int tabY =
            layout::kChromeBarPaddingY + (layout::kChromeBarTopRowHeight - layout::kChromeBarSubNavHeight) / 2;

        for (int i = 0; i < kVisibleDesignSectionCount; ++i)
        {
            designSubNavBounds_[static_cast<std::size_t>(i)] =
                {tabX, tabY, kDesignSectionWidths[i], layout::kChromeBarSubNavHeight};
            tabX += kDesignSectionWidths[i] + layout::kChromeSectionLedGap;
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
            if (designSubNavBounds_[static_cast<std::size_t>(i)].contains(pos))
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
                    onPlayViewModeChanged(layout::PlayViewMode::Basic);
            }
            else if (viewTab == static_cast<int>(ViewTab::Design))
            {
                if (onEditorModeChanged)
                    onEditorModeChanged(layout::EditorMode::Design);
            }
            return;
        }

        const int subNav = designSubNavIndexAt(event.getPosition());
        if (subNav < 0 || editorMode_ != layout::EditorMode::Design)
            return;

        const auto page = static_cast<layout::DesignSubPage>(subNav);
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

        if (onPlayViewModeChanged)
        {
            const auto next = playViewMode_ == layout::PlayViewMode::Basic ? layout::PlayViewMode::Advanced
                                                                           : layout::PlayViewMode::Basic;
            onPlayViewModeChanged(next);
        }
    }

    void MurmurChromeBar::paintBrand(juce::Graphics& g, juce::Rectangle<int> area) const
    {
        if (isCompactChrome())
        {
            g.setFont(fonts::label(10.0f));
            g.setColour(palette::kFigmaTextPrimary);
            g.drawText("MURMUR", area, juce::Justification::centredLeft, true);
            return;
        }

        g.setFont(fonts::label(16.0f));
        g.setColour(palette::kFigmaTextPrimary);
        g.drawText("MURMUR", area.removeFromTop(22), juce::Justification::centredLeft, true);

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kFigmaTextDim);
        const juce::String sub = editorMode_ == layout::EditorMode::Design
                                     ? "8-ENGINE · DISCRETE HYBRID SYNTHESIZER"
                                     : "8-ENGINE · DISCRETE HYBRID SYNTHESIZER";
        g.drawText(sub, area, juce::Justification::centredLeft, true);
    }

    void MurmurChromeBar::paintViewTabs(juce::Graphics& g)
    {
        const bool compactChrome = isCompactChrome();
        const auto active = activeViewTab();

        static constexpr const char* kDesktopLabels[] = {"COMPACT", "PLAY", "DESIGN"};
        static constexpr const char* kCompactLabels[] = {"CMP", "PLY", "DSN"};

        for (int i = 0; i < 3; ++i)
        {
            const auto tab = viewTabBounds_[static_cast<std::size_t>(i)];
            if (tab.isEmpty())
                continue;

            const juce::String label = compactChrome ? kCompactLabels[i] : kDesktopLabels[i];
            paintLedNavItem(g, tab, label, static_cast<ViewTab>(i) == active, compactChrome);
        }
    }

    void MurmurChromeBar::paintDesignSubNav(juce::Graphics& g)
    {
        if (editorMode_ != layout::EditorMode::Design || isCompactChrome())
            return;

        for (int i = 0; i < kVisibleDesignSectionCount; ++i)
        {
            const auto tab = designSubNavBounds_[static_cast<std::size_t>(i)];
            if (tab.isEmpty())
                continue;

            const bool active = static_cast<int>(designSubPage_) == i;
            paintLedNavItem(g, tab, kDesignSectionLabels[i], active, false);
        }
    }

    void MurmurChromeBar::paintScoreLine(juce::Graphics& g) const
    {
        if (isCompactChrome() || scoreLineBounds_.isEmpty())
            return;

        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.fillRect(scoreLineBounds_);
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
    }

    void MurmurChromeBar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        layoutViewTabBounds();
        layoutDesignSubNavBounds();

        auto content = getLocalBounds().reduced(layout::kChromeBarPaddingX, layout::kChromeBarPaddingY);
        if (isCompactChrome())
            content = getLocalBounds().reduced(10, 0);

        auto brandArea = content.removeFromLeft(isCompactChrome() ? 80 : layout::kChromeBarBrandWidth);
        paintBrand(g, brandArea);

        paintViewTabs(g);
        paintScoreLine(g);
        paintDesignSubNav(g);
        paintPresetBrowserChrome(g);
    }

    void MurmurChromeBar::resized()
    {
        layoutViewTabBounds();
        layoutDesignSubNavBounds();
        repaint();

        auto right = getLocalBounds().reduced(layout::kChromeBarPaddingX, layout::kChromeBarPaddingY);
        if (isCompactChrome())
        {
            right = getLocalBounds().reduced(10, 4);
            presetPrevBounds_ = {};
            presetNextBounds_ = {};
            browseButton_.setVisible(false);
            bpmLabel_.setVisible(false);
            masterOutLabel_.setVisible(false);
            masterVolumeKnob_->setVisible(false);
            presetMetaLabel_.setVisible(false);
            auto presetArea = juce::Rectangle<int>(252, 10, getWidth() - 262, 10);
            presetNameLabel_.setBounds(presetArea);
            presetDisplayBounds_ = presetArea;
            return;
        }

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
        presetDisplayBounds_ = presetBlock;

        presetNextBounds_ = presetBlock.removeFromRight(layout::kChromePresetChevronWidth);
        presetPrevBounds_ = presetBlock.removeFromLeft(layout::kChromePresetChevronWidth);

        auto metaRow = presetBlock.removeFromBottom(10);
        masterOutLabel_.setBounds(metaRow.removeFromRight(48));
        metaRow.removeFromRight(6);
        presetMetaLabel_.setBounds(metaRow);

        presetNameLabel_.setBounds(presetBlock.removeFromTop(14));
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetBlock);
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetNameLabel_.getBounds());
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetMetaLabel_.getBounds());
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetPrevBounds_);
        presetDisplayBounds_ = presetDisplayBounds_.getUnion(presetNextBounds_);

        if (editorMode_ == layout::EditorMode::Play && !scoreLineBounds_.isEmpty())
        {
            const int perfY = layout::kChromeBarPaddingY + 7;
            bpmLabel_.setBounds(scoreLineBounds_.getRight() + 16, perfY, 60, 13);
        }
    }

} // namespace pw8::plugin::ui
