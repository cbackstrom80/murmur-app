#include "MurmurRootEditor.h"

#include <algorithm>

#include "PlayModeLayout.h"
#include "net/MurmurApiClient.h"
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

    MurmurRootEditor::MurmurRootEditor(MurmurProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          patchBrowserBar_(processor),
          murmurChromeBar_(processor),
          presetBrowserOverlay_(processor, patchBrowserBar_.getPresetIndex(), favoritesStore_, ratingsStore_),
          playModeEditor_(processor, chrome_),
          designModeEditor_(processor, chrome_)
    {
        setLookAndFeel(&lookAndFeel_);

        aspectConstrainer_.setFixedAspectRatio(0.0);
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
            if (mode == layout::PlayViewMode::Advanced)
            {
                setEditorMode(layout::EditorMode::Design);
                setDesignSubPage(layout::DesignSubPage::Engine);
                return;
            }

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
            if (playModeEditor_.isCompactView())
                return;

            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
        };

        playModeEditor_.onLayoutOrViewModeChanged = [this] {
            syncChromeState();
            applyWindowConstraints();
            resized();
        };

        playModeEditor_.onEditorModeChangeRequested = [this](layout::EditorMode mode) { setEditorMode(mode); };
        playModeEditor_.onDesignSubPageChangeRequested = [this](layout::DesignSubPage page) {
            setEditorMode(layout::EditorMode::Design);
            setDesignSubPage(page);
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
        designModeEditor_.onOpenPlayFilterRequested = [this] {
            setEditorMode(layout::EditorMode::Play);
            playModeEditor_.setPlayViewMode(layout::PlayViewMode::Desktop);
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

        processor.onPatchDirtyChanged = [this] { murmurChromeBar_.refreshPresetDisplay(); };

        playModeEditor_.setPlayViewMode(layout::PlayViewMode::Desktop);
        syncChromeState();
        applyWindowConstraints();
        setSize(layout::kDefaultWidth, layout::defaultPlayRootHeight(layout::PlayViewMode::Desktop));

        // Key activation goes above everything but under the splash -- shown
        // once the splash clears, only if no license is stored locally yet
        // (content::LicenseStore's own file, checked fresh here rather than
        // cached, so a license activated in another editor instance is
        // picked up too).
        addChildComponent(keyActivationOverlay_);
        keyActivationOverlay_.setBounds(getLocalBounds());
        keyActivationOverlay_.onDismissed = [this] { keyActivationOverlay_.setVisible(false); };
        keyActivationOverlay_.onLibraryFetched = [this](const juce::Array<net::LibraryPatch>& patches) {
            if (patches.isEmpty())
                return;

            // Sequential downloads on one background thread (polite to the
            // server, and there's no UI progress for this yet) into the real,
            // already-scanned user preset root (pw8::content::presetSearchRoots()
            // includes "<AppData>/MURMUR/Presets") -- refreshPresetIndex() below
            // picks them up the same way any other user-dropped file would be.
            juce::Thread::launch([this, patches] {
                const auto destDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                          .getChildFile("MURMUR/Presets/library");
                destDir.createDirectory();

                int downloaded = 0;
                for (const auto& patch : patches)
                {
                    if (patch.downloadUrl.isEmpty() || patch.slug.isEmpty())
                        continue;

                    const juce::URL url(patch.downloadUrl);
                    const auto options =
                        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(8000);
                    auto stream = url.createInputStream(options);
                    if (stream == nullptr)
                        continue;

                    const auto body = stream->readEntireStreamAsString();
                    if (body.isEmpty())
                        continue;

                    destDir.getChildFile(patch.slug + ".murmur").replaceWithText(body);
                    ++downloaded;
                }

                juce::MessageManager::callAsync([this, downloaded] {
                    if (downloaded > 0)
                        patchBrowserBar_.refreshPresetIndex();
                });
            });
        };

        // Splash goes last so it paints on top of everything else added above.
        addAndMakeVisible(splashOverlay_);
        splashOverlay_.setBounds(getLocalBounds());
        splashOverlay_.onDismissed = [this] {
            splashOverlay_.setVisible(false);

            const content::LicenseStore licenseCheck;
            if (!licenseCheck.info().isActivated())
                keyActivationOverlay_.setVisible(true);
        };
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
            if (playModeEditor_.isCompactView())
                return;

            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
        };
        presetBrowserOverlay_.onClosed = [this] {
            patchBrowserBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            murmurChromeBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
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
            murmurChromeBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            playModeEditor_.setBrowseFilter(presetBrowserOverlay_.browseFilter());
        };
    }

    void MurmurRootEditor::setEditorMode(layout::EditorMode mode)
    {
        if (editorMode_ == mode)
            return;

        const auto previousMode = editorMode_;
        editorMode_ = mode;

        if (mode == layout::EditorMode::Design)
            designModeEditor_.setDesignSubPage(designSubPage_);

        if (mode == layout::EditorMode::Play && previousMode == layout::EditorMode::Design)
        {
            playModeEditor_.setPlayViewMode(lastNonCompactPlayView_);
            playModeEditor_.syncIpadFooterPill();
        }

        syncChromeState();
        applyWindowConstraints();

        if (mode == layout::EditorMode::Design)
            deferRootEditorSize(this, layout::kDefaultWidth, layout::minimumDesignRootHeight());
        else if (previousMode == layout::EditorMode::Design)
        {
            deferRootEditorSize(this, layout::kDefaultWidth,
                                layout::defaultPlayRootHeight(playModeEditor_.getPlayViewMode()));
        }

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

        if (playModeEditor_.getPlayViewMode() == layout::PlayViewMode::Basic
            || playModeEditor_.getPlayViewMode() == layout::PlayViewMode::Desktop)
            return layout::kDesktopPlayModeOuterMargin;

        return layout::kOuterMargin;
    }

    juce::BorderSize<int> MurmurRootEditor::contentInsetsForCurrentView() const
    {
        if (editorMode_ == layout::EditorMode::Design)
        {
            const int m = layout::kDesignModeV2OuterMargin;
            return {m, m, m, m};
        }

        if (playModeEditor_.isCompactView())
        {
            return {layout::kCompactOuterMargin, layout::kCompactOuterMargin, layout::kCompactBottomMargin,
                    layout::kCompactOuterMargin};
        }

        if (playModeEditor_.getPlayViewMode() == layout::PlayViewMode::Basic
            || playModeEditor_.getPlayViewMode() == layout::PlayViewMode::Desktop)
        {
            const int m = layout::kDesktopPlayModeOuterMargin;
            return {m, m, m, m};
        }

        const int m = layout::kOuterMargin;
        return {m, m, m, m};
    }

    void MurmurRootEditor::applyWindowConstraints()
    {
        if (editorMode_ == layout::EditorMode::Design)
        {
            const int minH = layout::minimumDesignRootHeight();
            setConstrainer(&aspectConstrainer_);
            aspectConstrainer_.setMinimumSize(layout::kMinWidth, minH);
            setResizeLimits(layout::kMinWidth, minH, layout::kMaxWidth, layout::kMaxHeight);
            if (getWidth() < layout::kMinWidth || getHeight() < minH)
                deferRootEditorSize(this, std::max(getWidth(), layout::kDefaultWidth),
                                    layout::clampRootHeight(getHeight(), editorMode_, playModeEditor_.getPlayViewMode(),
                                                            false));
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

        const auto playMode = playModeEditor_.getPlayViewMode();
        const int minH = layout::minimumPlayRootHeight(playMode);
        setConstrainer(&aspectConstrainer_);
        aspectConstrainer_.setMinimumSize(layout::kMinWidth, minH);
        setResizeLimits(layout::kMinWidth, minH, layout::kMaxWidth, layout::kMaxHeight);

        if (getWidth() < layout::kMinWidth || getHeight() < minH)
        {
            deferRootEditorSize(this, std::max(getWidth(), layout::kDefaultWidth),
                                layout::clampRootHeight(std::max(getHeight(), layout::defaultPlayRootHeight(playMode)),
                                                        editorMode_, playMode, false));
        }
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

        if (splashOverlay_.isVisible())
            splashOverlay_.setBounds(bounds);

        if (keyActivationOverlay_.isVisible())
            keyActivationOverlay_.setBounds(bounds);

        if (presetBrowserOverlay_.isVisible())
            presetBrowserOverlay_.setBounds(bounds);

        const int outerMargin = outerMarginForCurrentView();
        juce::ignoreUnused(outerMargin);
        const auto insets = contentInsetsForCurrentView();
        auto content = bounds;
        content.removeFromTop(insets.getTop());
        content.removeFromBottom(insets.getBottom());
        content.removeFromLeft(insets.getLeft());
        content.removeFromRight(insets.getRight());

        const bool compact = playModeEditor_.isCompactView();
        const int chromeHeight = layout::chromeBarHeight(editorMode_, compact);
        murmurChromeBar_.setBounds(content.removeFromTop(chromeHeight));

        const int sectionGap =
            compact ? layout::kCompactBlockGap
                    : (editorMode_ == layout::EditorMode::Design ? layout::kDesignModeV2SectionGap
                                                                 : (layout::isDesktopPlayLayout(playModeEditor_.getPlayViewMode())
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

        const bool compactPlay = playModeEditor_.isCompactView();

        if (!compactPlay && !presetBrowserOverlay_.isVisible()
            && (key == juce::KeyPress('b', juce::ModifierKeys::commandModifier, 0)
                || key == juce::KeyPress('b', juce::ModifierKeys::ctrlModifier, 0)))
        {
            addAndMakeVisible(presetBrowserOverlay_);
            presetBrowserOverlay_.setBounds(getLocalBounds());
            presetBrowserOverlay_.showOverlay();
            return true;
        }

        if (!compactPlay && !presetBrowserOverlay_.isVisible() && editorMode_ == layout::EditorMode::Play)
        {
            if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
            {
                murmurChromeBar_.stepPreset(key == juce::KeyPress::rightKey ? 1 : -1);
                return true;
            }
        }

        if (editorMode_ == layout::EditorMode::Design)
            return false;

        return playModeEditor_.keyPressed(key);
    }

} // namespace pw8::plugin::ui
