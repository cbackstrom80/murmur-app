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

        void setBrowseFilters(const juce::String& query, const juce::String& category, bool favoritesOnly = false);

        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }

        [[nodiscard]] bool browseFavoritesOnly() const { return browseFavoritesOnly_; }

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
        juce::String browseQuery_;
        juce::String browseCategory_;
        bool browseFavoritesOnly_ = false;
        content::FavoritesStore* favoritesStore_ = nullptr;
        std::unique_ptr<juce::FileChooser> fileChooser_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchBrowserBar)
    };

} // namespace pw8::plugin::ui
