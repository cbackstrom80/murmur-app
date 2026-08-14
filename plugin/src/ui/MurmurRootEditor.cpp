#include "MurmurRootEditor.h"

#include "PlayModeLayout.h"
#include "theme/BrandingAssets.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    MurmurRootEditor::MurmurRootEditor(PatchworkEightProcessor& processor)
        : juce::AudioProcessorEditor(&processor),
          patchBrowserBar_(processor),
          presetBrowserOverlay_(processor, patchBrowserBar_.getPresetIndex(), favoritesStore_),
          playModeEditor_(processor, chrome_)
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

        playModeEditor_.onLayoutOrViewModeChanged = [this] {
            syncChromeVisibility();
            applyWindowConstraints();
            resized();
        };

        addAndMakeVisible(patchBrowserBar_);
        addAndMakeVisible(playModeEditor_);

        processor.onPatchLoaded = [this, &processor] {
            juce::ignoreUnused(processor);
            playModeEditor_.refreshFromPatch();
        };

        processor.onPatchMetadataChanged = [this] { playModeEditor_.refreshFromPatch(); };

        syncChromeVisibility();
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
        };
        presetBrowserOverlay_.onFiltersChanged = [this] {
            patchBrowserBar_.setBrowseFilters(presetBrowserOverlay_.browseFilter());
            playModeEditor_.setBrowseFilter(presetBrowserOverlay_.browseFilter());
        };
    }

    void MurmurRootEditor::syncChromeVisibility()
    {
        patchBrowserBar_.setVisible(!playModeEditor_.isCompactView());
    }

    void MurmurRootEditor::applyWindowConstraints()
    {
        if (playModeEditor_.isCompactView())
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

        playModeEditor_.setBounds(content);
    }

    bool MurmurRootEditor::keyPressed(const juce::KeyPress& key)
    {
        return playModeEditor_.keyPressed(key);
    }

} // namespace pw8::plugin::ui
