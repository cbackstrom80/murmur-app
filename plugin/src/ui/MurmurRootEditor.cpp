#include "MurmurRootEditor.h"

#include "PlayModeLayout.h"
#include "theme/BrandingAssets.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        void deferRootEditorSize(MurmurRootEditor* editor, int width, int height)
        {
            juce::Component::SafePointer<MurmurRootEditor> safe(editor);
            juce::MessageManager::callAsync([safe, width, height]() {
                if (safe == nullptr)
                    return;
                if (safe->getWidth() == width && safe->getHeight() == height)
                    return;
                safe->setSize(width, height);
            });
        }
    } // namespace

    MurmurRootEditor::MurmurRootEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          patchBrowserBar_(processor),
          murmurChromeBar_(processor),
          presetBrowserOverlay_(processor, patchBrowserBar_.getPresetIndex(), favoritesStore_),
          playModeEditor_(processor, chrome_),
          designModeEditor_(processor, chrome_)
    {
        setLookAndFeel(&lookAndFeel_);

        aspectConstrainer_.setFixedAspectRatio(layout::kAspectRatio);
        aspectConstrainer_.setMinimumSize(layout::kMinWidth, layout::kMinHeight);
        aspectConstrainer_.setMaximumSize(layout::kMaxWidth, layout::kMaxHeight);

        compactConstrainer_.setFixedAspectRatio(0.0);
        compactConstrainer_.setMinimumSize(layout::kCompactWidth, layout::kCompactMinHeight);
        compactConstrainer_.setMaximumSize(layout::kCompactWidth, layout::kCompactMaxHeight);

        setConstrainer(&aspectConstrainer_);
        setResizeLimits(layout::kMinWidth, layout::kMinHeight, layout::kMaxWidth, layout::kMaxHeight);
        setResizable(true, true);

        wireSharedChrome();

        murmurChromeBar_.setPresetIndex(&patchBrowserBar_.getPresetIndex());
        murmurChromeBar_.setFavoritesStore(&favoritesStore_);
        murmurChromeBar_.onPlayViewModeChanged = [this](layout::PlayViewMode mode) {
            if (mode != layout::PlayViewMode::Compact)
                lastNonCompactPlayView_ = mode;
            playModeEditor_.setPlayViewMode(mode);
            syncChromeState();
            applyWindowConstraints();
            resized();
        };
        murmurChromeBar_.onEditorModeChanged = [this](layout::EditorMode mode) { setEditorMode(mode); };
        murmurChromeBar_.onDesignSubPageChanged = [this](layout::DesignSubPage page) { setDesignSubPage(page); };
        murmurChromeBar_.onBrowseRequested = [this] {
            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
        };

        playModeEditor_.onLayoutOrViewModeChanged = [this] {
            syncChromeState();
            applyWindowConstraints();
            resized();
        };

        addAndMakeVisible(murmurChromeBar_);
        addChildComponent(patchBrowserBar_);
        addAndMakeVisible(playModeEditor_);
        addChildComponent(designModeEditor_);

        designModeEditor_.onDesignSubPageChanged = [this](layout::DesignSubPage page) {
            if (designSubPage_ == page)
                return;
            designSubPage_ = page;
            murmurChromeBar_.setDesignSubPage(page);
        };

        processor.onPatchLoaded = [this, &processor] {
            juce::ignoreUnused(processor);
            playModeEditor_.refreshFromPatch();
            designModeEditor_.refreshFromPatch();
            murmurChromeBar_.refreshPresetDisplay();
        };

        processor.onPatchMetadataChanged = [this] {
            playModeEditor_.refreshFromPatch();
            designModeEditor_.refreshFromPatch();
            murmurChromeBar_.refreshPresetDisplay();
        };

        playModeEditor_.setPlayViewMode(layout::PlayViewMode::Basic);
        syncChromeState();
        applyWindowConstraints();
        setSize(layout::kDefaultWidth, layout::kDefaultHeight);
    }

    MurmurRootEditor::~MurmurRootEditor()
    {
        setConstrainer(nullptr);
        setLookAndFeel(nullptr);
    }

    void MurmurRootEditor::wireSharedChrome()
    {
        patchBrowserBar_.setFavoritesStore(&favoritesStore_);
        patchBrowserBar_.onBrowseClicked = [this] {
            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
        };
        presetBrowserOverlay_.onClosed = [this] {
            patchBrowserBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            removeChildComponent(&presetBrowserOverlay_);
            if (designSubPage_ == layout::DesignSubPage::Browse)
            {
                designSubPage_ = layout::DesignSubPage::Engine;
                murmurChromeBar_.setDesignSubPage(designSubPage_);
                designModeEditor_.setDesignSubPage(designSubPage_);
            }
        };
        presetBrowserOverlay_.onFiltersChanged = [this] {
            patchBrowserBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            playModeEditor_.setBrowseFilter(presetBrowserOverlay_.browseFilter());
        };
    }

    void MurmurRootEditor::setEditorMode(layout::EditorMode mode)
    {
        if (editorMode_ == mode)
            return;

        const auto previousMode = editorMode_;
        editorMode_ = mode;

        if (mode == layout::EditorMode::Play && previousMode == layout::EditorMode::Design)
            playModeEditor_.setPlayViewMode(lastNonCompactPlayView_);

        syncChromeState();
        applyWindowConstraints();
        resized();
    }

    void MurmurRootEditor::setDesignSubPage(layout::DesignSubPage page)
    {
        if (page == layout::DesignSubPage::Browse)
        {
            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
            return;
        }

        if (designSubPage_ == page)
            return;

        designSubPage_ = page;
        designModeEditor_.setDesignSubPage(page);
        murmurChromeBar_.setDesignSubPage(page);
    }

    void MurmurRootEditor::syncChromeState()
    {
        const bool design = editorMode_ == layout::EditorMode::Design;
        const bool compact = !design && playModeEditor_.isCompactView();

        designModeEditor_.setVisible(design);
        playModeEditor_.setVisible(!design);
        patchBrowserBar_.setVisible(false);

        murmurChromeBar_.setEditorMode(editorMode_);
        murmurChromeBar_.setPlayViewMode(playModeEditor_.getPlayViewMode());
        murmurChromeBar_.setDesignSubPage(designSubPage_);
        murmurChromeBar_.setCompactWindowMode(compact);
    }

    int MurmurRootEditor::outerMarginForCurrentView() const
    {
        if (editorMode_ == layout::EditorMode::Design)
            return layout::kDesignModeV2OuterMargin;

        if (playModeEditor_.isCompactView())
            return layout::kCompactOuterMargin;

        if (playModeEditor_.getPlayViewMode() == layout::PlayViewMode::Basic)
            return layout::kDesktopPlayModeOuterMargin;

        return layout::kOuterMargin;
    }

    void MurmurRootEditor::applyWindowConstraints()
    {
        if (editorMode_ == layout::EditorMode::Design)
        {
            setConstrainer(&aspectConstrainer_);
            setResizeLimits(layout::kMinWidth, layout::kMinHeight, layout::kMaxWidth, layout::kMaxHeight);
            if (getWidth() < layout::kMinWidth || getHeight() < layout::kMinHeight)
                deferRootEditorSize(this, layout::kDefaultWidth, layout::kDefaultHeight);
            return;
        }

        if (playModeEditor_.isCompactView())
        {
            setConstrainer(&compactConstrainer_);
            setResizeLimits(layout::kCompactWidth, layout::kCompactMinHeight, layout::kCompactWidth,
                            layout::kCompactMaxHeight);
            if (getWidth() != layout::kCompactWidth || getHeight() < layout::kCompactMinHeight)
                deferRootEditorSize(this, layout::kCompactWidth, layout::kCompactDefaultHeight);
            return;
        }

        setConstrainer(&aspectConstrainer_);
        setResizeLimits(layout::kMinWidth, layout::kMinHeight, layout::kMaxWidth, layout::kMaxHeight);
        if (getWidth() < layout::kMinWidth || getHeight() < layout::kMinHeight)
            deferRootEditorSize(this, layout::kDefaultWidth, layout::kDefaultHeight);
    }

    void MurmurRootEditor::paint(juce::Graphics& g)
    {
        const auto h = static_cast<float>(getHeight());
        juce::ColourGradient bg(palette::kFigmaBgDeep, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f, h, false);
        g.setGradientFill(bg);
        g.fillAll();
    }

    void MurmurRootEditor::resized()
    {
        auto bounds = getLocalBounds();

        if (presetBrowserOverlay_.isVisible())
            presetBrowserOverlay_.setBounds(bounds);

        const int outerMargin = outerMarginForCurrentView();
        auto content = bounds.reduced(outerMargin);

        const bool compact = playModeEditor_.isCompactView();
        const int chromeHeight = layout::chromeBarHeight(editorMode_, compact);
        murmurChromeBar_.setBounds(content.removeFromTop(chromeHeight));

        const int sectionGap =
            compact ? layout::kCompactBlockGap
                    : (editorMode_ == layout::EditorMode::Design ? layout::kDesignModeV2SectionGap
                                                                 : (playModeEditor_.getPlayViewMode()
                                                                            == layout::PlayViewMode::Basic
                                                                        ? layout::kDesktopPlayModeSectionGap
                                                                        : layout::kSectionGap));
        content.removeFromTop(sectionGap);

        if (editorMode_ == layout::EditorMode::Design)
            designModeEditor_.setBounds(content);
        else
            playModeEditor_.setBounds(content);
    }

    bool MurmurRootEditor::keyPressed(const juce::KeyPress& key)
    {
        if (presetBrowserOverlay_.isVisible() && presetBrowserOverlay_.keyPressed(key))
            return true;

        if (!presetBrowserOverlay_.isVisible()
            && (key == juce::KeyPress('b', juce::ModifierKeys::commandModifier, 0)
                || key == juce::KeyPress('b', juce::ModifierKeys::ctrlModifier, 0)))
        {
            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
            return true;
        }

        if (editorMode_ == layout::EditorMode::Design)
            return false;

        return playModeEditor_.keyPressed(key);
    }

} // namespace pw8::plugin::ui
