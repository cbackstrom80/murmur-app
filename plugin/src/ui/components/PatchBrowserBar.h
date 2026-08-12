#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    class PatchBrowserBar : public juce::Component, private juce::Timer
    {
    public:
        explicit PatchBrowserBar(PatchworkEightProcessor& processor);
        ~PatchBrowserBar() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void refreshPresetIndex();

        [[nodiscard]] content::PresetIndex& getPresetIndex() { return presetIndex_; }
        [[nodiscard]] const content::PresetIndex& getPresetIndex() const { return presetIndex_; }

        void setBrowseFilters(const content::PresetMetadataFilter& filter);

        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }

        [[nodiscard]] const content::PresetMetadataFilter& browseFilter() const { return browseFilter_; }

        std::function<void()> onBrowseClicked;

    private:
        void timerCallback() override;
        void loadPatchFromFile();
        void stepPreset(int direction);

        PatchworkEightProcessor& processor_;
        content::PresetIndex presetIndex_;
        juce::Label patchNameLabel_;
        juce::TextButton prevButton_{"<"};
        juce::TextButton nextButton_{">"};
        juce::TextButton browseButton_{"BROWSE"};
        juce::TextButton loadButton_{"LOAD..."};
        content::PresetMetadataFilter browseFilter_;
        content::FavoritesStore* favoritesStore_ = nullptr;
        std::unique_ptr<juce::FileChooser> fileChooser_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowserBar)
    };

} // namespace pw8::plugin::ui
