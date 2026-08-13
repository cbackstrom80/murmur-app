#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "CircularSpectrumScope.h"
#include "ScopeModeToggle.h"
#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "GlowKnob.h"
#include "PatchFocusPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    /// 320px-wide performance strip: patch header, circular FFT, volume, Knobs of Interest.
    class CompactModeEditor : public juce::Component, private juce::Timer
    {
    public:
        explicit CompactModeEditor(PatchworkEightProcessor& processor);

        ~CompactModeEditor() override;

        void resized() override;

        void setPresetIndex(content::PresetIndex* presetIndex) { presetIndex_ = presetIndex; }

        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }

        void setBrowseFilter(const content::PresetMetadataFilter& filter) { browseFilter_ = filter; }

        void refreshFromPatch() { focusPanel_.refreshFromPatch(); }

    private:
        void timerCallback() override;
        void stepPreset(int direction);

        PatchworkEightProcessor& processor_;
        content::PresetIndex* presetIndex_ = nullptr;
        content::FavoritesStore* favoritesStore_ = nullptr;
        content::PresetMetadataFilter browseFilter_;

        juce::Label patchNameLabel_;
        juce::TextButton prevButton_{"<"};
        juce::TextButton nextButton_{">"};
        CircularSpectrumScope circularScope_;
        ScopeModeToggle scopeModeToggle_;
        std::unique_ptr<GlowKnob> volumeKnob_;
        PatchFocusPanel focusPanel_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactModeEditor)
    };

} // namespace pw8::plugin::ui
