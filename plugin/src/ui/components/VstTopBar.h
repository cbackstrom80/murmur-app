#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PlayModeLayout.h"
#include "ArpLauncherChip.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma obsidian-play-board mode row: Basic / Advanced / Compact + ARP launcher.
    class VstTopBar : public juce::Component
    {
    public:
        explicit VstTopBar(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;

        void setPlayViewMode(layout::PlayViewMode mode);
        [[nodiscard]] layout::PlayViewMode getPlayViewMode() const noexcept { return viewMode_; }

        void setEditorMode(layout::EditorMode mode);
        [[nodiscard]] layout::EditorMode getEditorMode() const noexcept { return editorMode_; }

        /// Compact teleprompter window: hide Advanced + ARP chip.
        void setCompactWindowMode(bool compactWindow);

        /// Overlay inside unified 54px chrome — skip duplicate panel chrome.
        void setUnifiedChromeMode(bool unifiedChrome);

        std::function<void(layout::PlayViewMode)> onViewModeChanged;
        std::function<void(layout::EditorMode)> onEditorModeChanged;
        std::function<void()> onArpRequested;

    private:
        void applyViewModeUi();

        layout::EditorMode editorMode_ = layout::EditorMode::Play;
        layout::PlayViewMode viewMode_ = layout::PlayViewMode::Basic;
        bool compactWindow_ = false;
        bool unifiedChromeMode_ = false;

        ArpLauncherChip arpLauncherChip_;
        juce::TextButton playModeTabButton_{"PLAY"};
        juce::TextButton designModeTabButton_{"DESIGN"};
        juce::TextButton basicViewButton_{"Basic"};
        juce::TextButton advancedViewButton_{"Advanced"};
        std::unique_ptr<juce::TextButton> compactViewButton_;
    };

} // namespace pw8::plugin::ui
