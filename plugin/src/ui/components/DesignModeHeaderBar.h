#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PlayModeLayout.h"
#include "GlowKnob.h"
#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-design-mode-v2` header-bar (37:788): brand, PLAY/DESIGN/MOD/FX/BROWSE, preset, master.
    class DesignModeHeaderBar : public juce::Component, private juce::Timer
    {
    public:
        explicit DesignModeHeaderBar(PatchworkEightProcessor& processor);
        ~DesignModeHeaderBar() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        void setEditorMode(layout::EditorMode mode);
        void setFavoritesStore(content::FavoritesStore* favoritesStore) { favoritesStore_ = favoritesStore; }
        void refreshPresetIndex();

        std::function<void(layout::EditorMode)> onEditorModeChanged;
        std::function<void()> onBrowseClicked;

    private:
        void timerCallback() override;
        void stepPreset(int direction);
        [[nodiscard]] int navTabIndexAt(juce::Point<int> pos) const;

        PatchworkEightProcessor& processor_;
        content::PresetIndex presetIndex_;
        content::FavoritesStore* favoritesStore_ = nullptr;
        layout::EditorMode editorMode_ = layout::EditorMode::Design;

        juce::Label presetNameLabel_;
        juce::Label presetBankLabel_;
        std::unique_ptr<GlowKnob> masterVolumeKnob_;
        std::array<juce::Rectangle<int>, 5> navTabBounds_{};
    };

} // namespace pw8::plugin::ui
