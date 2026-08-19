#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "GlowKnob.h"
#include "HeaderSpectrumScope.h"
#include "ScopeModeToggle.h"
#include "processor/MurmurProcessor.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    class PatchBrowserBar : public juce::Component, private juce::Timer
    {
    public:
        explicit PatchBrowserBar(MurmurProcessor& processor);
        ~PatchBrowserBar() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        void refreshPresetIndex();

        [[nodiscard]] content::PresetIndex& getPresetIndex() { return presetIndex_; }
        [[nodiscard]] const content::PresetIndex& getPresetIndex() const { return presetIndex_; }

        void setBrowseFilters(const content::PresetMetadataFilter& filter);

        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }

        [[nodiscard]] const content::PresetMetadataFilter& browseFilter() const { return browseFilter_; }

        /// Figma obsidian-play-board unified 54px chrome (22:3).
        void setObsidianChromeMode(bool obsidianChrome);

        /// Figma murmur-desktop-play-mode top-bar (36:5) — 72px performance chrome.
        void setDesktopPlayModeChrome(bool desktopChrome);

        std::function<void()> onBrowseClicked;
        /// Double-click brand in desktop chrome — escape hatch when VstTopBar is hidden.
        std::function<void()> onExitDesktopChromeRequested;

    private:
        void timerCallback() override;
        void loadPatchFromFile();
        void stepPreset(int direction);
        void paintDesktopPlayModeChrome(juce::Graphics& g, juce::Rectangle<float> bounds);

        MurmurProcessor& processor_;
        content::PresetIndex presetIndex_;
        juce::Label patchNameLabel_;
        juce::Label patchHintLabel_;
        juce::TextButton prevButton_{"<"};
        juce::TextButton nextButton_{">"};
        juce::TextButton browseButton_{"BROWSE"};
        juce::TextButton loadButton_{"LOAD..."};
        HeaderSpectrumScope spectrumScope_;
        ScopeModeToggle scopeModeToggle_;
        std::unique_ptr<GlowKnob> masterVolumeKnob_;
        content::PresetMetadataFilter browseFilter_;
        content::FavoritesStore* favoritesStore_ = nullptr;
        std::unique_ptr<juce::FileChooser> fileChooser_;
        juce::String lastPresetCategory_;
        juce::String desktopPresetBankText_;
        float desktopHostBpm_ = 120.0f;
        float desktopMasterGain_ = 1.0f;
        bool obsidianChromeMode_ = false;
        bool desktopPlayModeChrome_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowserBar)
    };

} // namespace pw8::plugin::ui
