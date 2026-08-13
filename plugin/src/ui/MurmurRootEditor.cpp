#include "MurmurRootEditor.h"

#include "PlayModeLayout.h"
#include "theme/BrandingAssets.h"
#include "theme/ObsidianFonts.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    MurmurRootEditor::MurmurRootEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          patchBrowserBar_(processor),
          presetBrowserOverlay_(processor, patchBrowserBar_.getPresetIndex(), favoritesStore_),
          playModeEditor_(processor, chrome_),
          designModeEditor_()
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

        for (auto* btn : {&playModeButton_, &designModeButton_})
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(9050);
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::buttonOnColourId, palette::kAccentDim.withAlpha(0.55f));
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn->setColour(juce::TextButton::textColourOnId, palette::kAccent);
            addAndMakeVisible(*btn);
        }
        playModeButton_.onClick = [this] { setAppMode(AppMode::Play); };
        designModeButton_.onClick = [this] { setAppMode(AppMode::Design); };

        playModeEditor_.onLayoutOrViewModeChanged = [this] {
            syncChromeVisibility();
            applyWindowConstraints();
            resized();
        };

        addAndMakeVisible(patchBrowserBar_);
        addAndMakeVisible(playModeEditor_);
        addChildComponent(designModeEditor_);

        processor.onPatchLoaded = [this, &processor] {
            juce::ignoreUnused(processor);
            playModeEditor_.refreshFromPatch();
        };

        setAppMode(AppMode::Play);
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
        };
        presetBrowserOverlay_.onFiltersChanged = [this] {
            patchBrowserBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            playModeEditor_.setBrowseFilter(presetBrowserOverlay_.browseFilter());
        };
    }

    void MurmurRootEditor::setAppMode(AppMode mode)
    {
        appMode_ = mode;
        playModeButton_.setToggleState(mode == AppMode::Play, juce::dontSendNotification);
        designModeButton_.setToggleState(mode == AppMode::Design, juce::dontSendNotification);

        playModeEditor_.setVisible(mode == AppMode::Play);
        designModeEditor_.setVisible(mode == AppMode::Design);

        syncChromeVisibility();
        applyWindowConstraints();
        resized();
    }

    void MurmurRootEditor::syncChromeVisibility()
    {
        const bool compactPlay = appMode_ == AppMode::Play && playModeEditor_.isCompactView();
        patchBrowserBar_.setVisible(!compactPlay);
        designModeButton_.setVisible(appMode_ == AppMode::Play || appMode_ == AppMode::Design);
        playModeButton_.setVisible(true);
    }

    void MurmurRootEditor::applyWindowConstraints()
    {
        if (appMode_ == AppMode::Play && playModeEditor_.isCompactView())
        {
            setConstrainer(&compactConstrainer_);
            setResizeLimits(layout::kCompactWidth, layout::kCompactMinHeight, layout::kCompactWidth,
                            layout::kCompactMaxHeight);
            if (getWidth() != layout::kCompactWidth || getHeight() < layout::kCompactMinHeight)
                setSize(layout::kCompactWidth, layout::kCompactDefaultHeight);
            return;
        }

        setConstrainer(&aspectConstrainer_);
        setResizeLimits(layout::kMinWidth, layout::kMinHeight, layout::kMaxWidth, layout::kMaxHeight);
        if (getWidth() < layout::kMinWidth || getHeight() < layout::kMinHeight)
            setSize(layout::kDefaultWidth, layout::kDefaultHeight);
    }

    void MurmurRootEditor::paint(juce::Graphics& g)
    {
        const auto h = static_cast<float>(getHeight());
        juce::ColourGradient bg(palette::kBackgroundTop, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f, h, false);
        g.setGradientFill(bg);
        g.fillAll();
    }

    void MurmurRootEditor::resized()
    {
        auto bounds = getLocalBounds();

        if (presetBrowserOverlay_.isVisible())
            presetBrowserOverlay_.setBounds(bounds);

        auto content = bounds.reduced(layout::kOuterMargin);

        if (patchBrowserBar_.isVisible())
        {
            patchBrowserBar_.setBounds(content.removeFromTop(branding::headerBarHeight()));
            content.removeFromTop(layout::kBlockGap);
        }

        auto modeRow = content.removeFromTop(layout::kViewModeRowHeight);
        playModeButton_.setBounds(modeRow.removeFromLeft(72).reduced(2));
        designModeButton_.setBounds(modeRow.removeFromLeft(88).reduced(2));
        content.removeFromTop(layout::kBlockGap);

        playModeEditor_.setBounds(content);
        designModeEditor_.setBounds(content);
    }

    bool MurmurRootEditor::keyPressed(const juce::KeyPress& key)
    {
        if (appMode_ == AppMode::Play)
            return playModeEditor_.keyPressed(key);
        return false;
    }

} // namespace pw8::plugin::ui
