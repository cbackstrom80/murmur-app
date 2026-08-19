#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/MurmurProcessor.h"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "../PlayModeLayout.h"

namespace pw8::plugin::ui
{
    class LabLauncherChip;

    class VstBottomBar : public juce::Component, private juce::Timer
    {
    public:
        explicit VstBottomBar(MurmurProcessor& processor);
        ~VstBottomBar() override;

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        /// Figma obsidian-play-board status-bar (22:414): VOCODER + LFO chips only.
        void setPlayBoardMode(bool playBoardMode);

        /// Figma `murmur-desktop-play-mode` vst-bottom-bar (36:226).
        void setDesktopPlayMode(bool desktopPlayMode);

        /// Figma `ipad-play-view` vst-bottom-bar (4:2629) — diagnostics, footer pills, panic.
        void setIpadPlayMode(bool ipadPlayMode);

        void setIpadFooterPillActive(layout::IpadFooterPill pill);

        /// Figma `murmur-design-mode-v2` status-bar (37:1507).
        void setDesignModeV2Layout(bool designMode);

        /// Figma `murmur-basic-view` bottom-bar (`86:225`) — 40px diagnostics strip.
        void setMurmurBasicViewMode(bool murmurBasicViewMode);

        std::function<void()> onVocoderLabRequested;
        std::function<void()> onQuasarLabRequested;
        std::function<void()> onLfoLabRequested;
        std::function<void()> onMotionLabRequested;
        std::function<void()> onMorphEditorRequested;
        std::function<void()> onFilterLabRequested;
        std::function<void()> onDynamicsLabRequested;
        std::function<void()> onGenerativeLabRequested;
        std::function<void()> onUtilityPeaksRequested;
        std::function<void()> onEnvelopeSegmentsRequested;
        std::function<void()> onModMatrixRequested;
        std::function<void(layout::IpadFooterPill)> onIpadFooterPillSelected;

    private:
        void timerCallback() override;
        void paintDesignModeModChips(juce::Graphics& g) const;
        void paintIpadFooterPills(juce::Graphics& g) const;
        [[nodiscard]] bool modSourceActive(modulation::ModSource source) const;
        [[nodiscard]] std::optional<layout::IpadFooterPill> footerPillAt(juce::Point<int> point) const;

        MurmurProcessor& processor_;
        juce::Label statusLabel_;
        juce::Label voicesLabel_;
        std::unique_ptr<LabLauncherChip> vocoderChip_;
        std::unique_ptr<LabLauncherChip> quasarChip_;
        std::unique_ptr<LabLauncherChip> lfoChip_;
        std::unique_ptr<LabLauncherChip> motionChip_;
        std::unique_ptr<LabLauncherChip> morphEditorChip_;
        std::unique_ptr<LabLauncherChip> filterLabChip_;
        std::unique_ptr<LabLauncherChip> dynamicsLabChip_;
        std::unique_ptr<LabLauncherChip> generativeLabChip_;
        std::unique_ptr<LabLauncherChip> utilityPeaksChip_;
        std::unique_ptr<LabLauncherChip> segmentsLabChip_;
        std::unique_ptr<LabLauncherChip> modChip_;
        juce::TextButton panicButton_{"PANIC"};
        juce::TextButton latchButton_{"HOLD / LATCH [ON]"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment_;
        juce::Label sustainLabel_;
        juce::Label midiLabel_;
        juce::Label routeLabel_;
        juce::Label cpuLabel_;
        juce::Label bpmLabel_;
        juce::Label fxLoadLabel_;
        juce::Label modSourcesLabel_;
        std::array<std::unique_ptr<juce::Label>, 4> modSourceChips_;
        std::array<juce::Rectangle<int>, 4> modChipBounds_{};
        juce::Rectangle<int> cpuBarBounds_;
        juce::Rectangle<int> fxLoadBarBounds_;
        std::array<juce::Rectangle<int>, 5> ipadFooterPillBounds_{};
        layout::IpadFooterPill activeIpadFooterPill_ = layout::IpadFooterPill::Play;
        float cpuLoadPercent_ = 0.0f;
        float fxLoadPercent_ = 0.0f;
        bool playBoardMode_ = false;
        bool desktopPlayMode_ = false;
        bool ipadPlayMode_ = false;
        bool designModeV2Layout_ = false;
        bool murmurBasicViewMode_ = false;
    };

} // namespace pw8::plugin::ui
