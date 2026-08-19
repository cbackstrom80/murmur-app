#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PlayModeLayout.h"
#include "GlowKnob.h"
#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Unified header — Figma `header-bar` on compact (39:158), play (39:142), design (39:2).
    class MurmurChromeBar : public juce::Component, private juce::Timer
    {
    public:
        explicit MurmurChromeBar(MurmurProcessor& processor);
        ~MurmurChromeBar() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;

        void setEditorMode(layout::EditorMode mode);
        void setPlayViewMode(layout::PlayViewMode mode);
        void setDesignSubPage(layout::DesignSubPage page);
        void setCompactWindowMode(bool compactWindow);
        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }
        void setPresetIndex(content::PresetIndex* presetIndex) { presetIndex_ = presetIndex; }

        void setBrowseFilters(const content::PresetMetadataFilter& filter);
        [[nodiscard]] const content::PresetMetadataFilter& browseFilter() const noexcept { return browseFilter_; }

        void stepPreset(int direction);

        void refreshPresetDisplay();

        std::function<void(layout::PlayViewMode)> onPlayViewModeChanged;
        std::function<void(layout::EditorMode)> onEditorModeChanged;
        std::function<void(layout::DesignSubPage)> onDesignSubPageChanged;
        std::function<void()> onBrowseRequested;

    private:
        enum class ViewTab
        {
            Compact,
            Play,
            Design,
        };

        void timerCallback() override;
        [[nodiscard]] ViewTab activeViewTab() const noexcept;
        [[nodiscard]] bool isCompactChrome() const noexcept;
        [[nodiscard]] bool isDesignTwoRow() const noexcept;
        [[nodiscard]] bool isPlaySubNavVisible() const noexcept;
        void paintViewTabs(juce::Graphics& g);
        void paintViewSectionDivider(juce::Graphics& g) const;
        void paintDesignSubNav(juce::Graphics& g);
        void paintPlaySubNav(juce::Graphics& g);
        void paintScoreLine(juce::Graphics& g) const;
        void paintBrand(juce::Graphics& g, juce::Rectangle<int> area) const;
        void paintCompactLogo(juce::Graphics& g) const;
        void layoutViewTabBounds();
        void layoutDesignSubNavBounds();
        void layoutPlaySubNavBounds();
        [[nodiscard]] int viewTabIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] int designSubNavIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] int playSubNavIndexAt(juce::Point<int> pos) const;
        [[nodiscard]] bool presetChevronAt(juce::Point<int> pos, int& direction) const;
        void updatePlayViewToggleButton();
        void promptSavePatch();
        void launchSaveAsCopyDialog();
        [[nodiscard]] bool savePatchButtonAt(juce::Point<int> pos) const;
        void paintSavePatchButton(juce::Graphics& g) const;

        [[nodiscard]] int navContentStartX() const noexcept
        {
            return layout::chromeBarNavStartX();
        }

        MurmurProcessor& processor_;
        content::PresetIndex* presetIndex_ = nullptr;
        content::FavoritesStore* favoritesStore_ = nullptr;
        content::PresetMetadataFilter browseFilter_;

        layout::EditorMode editorMode_ = layout::EditorMode::Play;
        layout::PlayViewMode playViewMode_ = layout::PlayViewMode::Desktop;
        layout::DesignSubPage designSubPage_ = layout::DesignSubPage::Engine;
        bool compactWindow_ = false;

        void paintPresetBrowserChrome(juce::Graphics& g) const;

        juce::Label presetNameLabel_;
        juce::Label presetMetaLabel_;
        juce::Label bpmLabel_;
        juce::Label masterOutLabel_;
        juce::TextButton browseButton_{"BROWSE"};
        juce::TextButton playViewToggleButton_{"ADVANCED"};
        std::unique_ptr<GlowKnob> masterVolumeKnob_;

        std::array<juce::Rectangle<int>, 3> viewTabBounds_{};
        std::array<juce::Rectangle<int>, 2> playSubNavBounds_{};
        std::array<juce::Rectangle<int>, 10> designSubNavBounds_{};
        juce::Rectangle<int> viewSectionDividerBounds_{};
        juce::Rectangle<int> scoreLineBounds_{};
        juce::Rectangle<int> presetPrevBounds_;
        juce::Rectangle<int> presetNextBounds_;
        juce::Rectangle<int> presetDisplayBounds_;
        juce::Rectangle<int> savePatchButtonBounds_;
        int frStepFlashTicks_ = 0;
        juce::String frStepKeyframeName_;
    };

} // namespace pw8::plugin::ui
