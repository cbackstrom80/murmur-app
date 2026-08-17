#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    class LabLauncherChip;

    class VstBottomBar : public juce::Component, private juce::Timer
    {
    public:
        explicit VstBottomBar(PatchworkEightProcessor& processor);
        ~VstBottomBar() override;

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        /// Figma obsidian-play-board status-bar (22:414): VOCODER + LFO chips only.
        void setPlayBoardMode(bool playBoardMode);

        /// Figma `murmur-desktop-play-mode` vst-bottom-bar (36:226).
        void setDesktopPlayMode(bool desktopPlayMode);

        /// Figma `murmur-design-mode-v2` status-bar (37:1507).
        void setDesignModeV2Layout(bool designMode);

        std::function<void()> onVocoderLabRequested;
        std::function<void()> onLfoLabRequested;
        std::function<void()> onModMatrixRequested;

    private:
        void timerCallback() override;
        void paintDesignModeModChips(juce::Graphics& g) const;
        [[nodiscard]] bool modSourceActive(modulation::ModSource source) const;

        PatchworkEightProcessor& processor_;
        juce::Label statusLabel_;
        juce::Label voicesLabel_;
        std::unique_ptr<LabLauncherChip> vocoderChip_;
        std::unique_ptr<LabLauncherChip> lfoChip_;
        std::unique_ptr<LabLauncherChip> modChip_;
        juce::TextButton panicButton_{"PANIC"};
        juce::TextButton latchButton_{"HOLD / LATCH [ON]"};
        juce::Label sustainLabel_;
        juce::Label midiLabel_;
        juce::Label routeLabel_;
        juce::Label cpuLabel_;
        juce::Label fxLoadLabel_;
        juce::Label modSourcesLabel_;
        std::array<std::unique_ptr<juce::Label>, 4> modSourceChips_;
        std::array<juce::Rectangle<int>, 4> modChipBounds_{};
        juce::Rectangle<int> cpuBarBounds_;
        juce::Rectangle<int> fxLoadBarBounds_;
        float cpuLoadPercent_ = 0.0f;
        float fxLoadPercent_ = 0.0f;
        bool playBoardMode_ = false;
        bool desktopPlayMode_ = false;
        bool designModeV2Layout_ = false;
    };

} // namespace pw8::plugin::ui
