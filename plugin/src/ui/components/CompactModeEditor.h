#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "CircularSpectrumScope.h"
#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "PatchFocusPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    /// 320px-wide performance teleprompter: mission card, circular scope hub, 1–3 feature macro KOINS orbit.
    class CompactModeEditor : public juce::Component, private juce::Timer
    {
    public:
        explicit CompactModeEditor(PatchworkEightProcessor& processor);

        ~CompactModeEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void setPresetIndex(content::PresetIndex* presetIndex) { presetIndex_ = presetIndex; }

        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }

        void setBrowseFilter(const content::PresetMetadataFilter& filter) { browseFilter_ = filter; }

        void refreshFromPatch() { focusPanel_.refreshFromPatch(); }

    private:
        void timerCallback() override;
        void stepPreset(int direction);
        void updateMissionCard();

        PatchworkEightProcessor& processor_;
        content::PresetIndex* presetIndex_ = nullptr;
        content::FavoritesStore* favoritesStore_ = nullptr;
        content::PresetMetadataFilter browseFilter_;

        juce::TextButton prevButton_{"<"};
        juce::TextButton nextButton_{">"};
        juce::Label missionNameLabel_;
        juce::Label missionCategoryLabel_;
        juce::Label missionHintLabel_;
        CircularSpectrumScope circularScope_;
        PatchFocusPanel focusPanel_;

        juce::String lastCategory_;
        juce::String lastHint_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactModeEditor)
    };

} // namespace pw8::plugin::ui
