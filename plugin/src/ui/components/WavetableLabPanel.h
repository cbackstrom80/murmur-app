#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "ConcentricGlowKnob.h"
#include "ModAssignmentController.h"
#include "TripleGlowKnob.h"
#include "SectionPanel.h"
#include "WavetableStackView.h"
#include "../PlayModeLayout.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Full-screen wavetable editor — Figma `murmur-wavetable-editor` (27:709).
    class WavetableLabPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit WavetableLabPanel(MurmurProcessor& processor, ModAssignmentController& assignmentController);
        ~WavetableLabPanel() override;

        std::function<void()> onClosed;

        void showForEngine(int engineIndex);
        void dismiss();

        void setEmbeddedInDesignMode(bool embedded);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void timerCallback() override;
        void bindEngine(int engineIndex);
        void wireModTargets();
        void ensureWavetableEngine();
        void paintHarmonicEditor(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintFrameStrip(juce::Graphics& g) const;
        void paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintOscConfigExtras(juce::Graphics& g) const;
        void layoutOscConfigPanel();
        void layoutFrameStrip();
        void refreshWaveformButtons();
        void refreshMorphTypeButtons();
        void refreshFrameStripSelection();
        void selectWaveformSource(int sourceIndex);
        void selectMorphType(int morphIndex);
        void selectFrameHighlight(int stripIndex);
        void refreshHarmonicHeights();
        [[nodiscard]] int activeWaveformSourceIndex() const;
        [[nodiscard]] int currentWavetableFrameIndex() const;
        [[nodiscard]] int wavetableFrameCount() const;

        void paintOverChildren(juce::Graphics& g) override;

        MurmurProcessor& processor_;
        ModAssignmentController& assignmentController_;
        juce::AudioProcessorValueTreeState& apvts_;
        int engineIndex_ = 0;

        juce::TextButton backButton_{"← PLAY BOARD"};
        juce::Label badgeLabel_;
        juce::Label titleLabel_;
        juce::ComboBox engineCombo_;
        juce::Label engineHintLabel_;
        juce::Label subtitleTitleLabel_;
        juce::Label subtitleMorphLabel_;
        juce::Label footerCpuLabel_;
        juce::Label footerCpuPercentLabel_;
        juce::Label footerVoicesLabel_;
        juce::Label footerVoicesCountLabel_;
        juce::Label footerMidiLabel_;
        juce::Label footerVersionLabel_;

        SectionPanel oscConfigPanel_{"OSCILLATOR CONFIG"};
        SectionPanel meshPanel_{"3D SPECTRUM PREVIEW"};
        SectionPanel morphPanel_{"MORPH CONTROLS"};
        WavetableStackView meshView_;
        SectionPanel harmonicPanel_{"HARMONIC PARTIALS (1-16)"};

        std::array<juce::TextButton, 6> waveformButtons_{};
        std::unique_ptr<ConcentricGlowKnob> pitchKnob_;
        juce::Label phaseDispersionLabel_;
        juce::TextButton phaseDispersionToggle_{"ON"};

        std::unique_ptr<GlowKnob> wtPosKnob_;
        std::unique_ptr<TripleGlowKnob> unisonKnob_;
        std::array<juce::TextButton, 3> morphTypeButtons_{};
        juce::Label morphPositionLabel_;
        juce::Label frameStripTitleLabel_;

        juce::Rectangle<int> frameStripBounds_;
        std::array<juce::Rectangle<int>, layout::kDesignWavetableFrameStripCount> frameMiniBounds_{};
        int morphTypeIndex_ = 0;
        int selectedFrameStripIndex_ = 4;

        juce::Rectangle<int> unisonKnobBounds_;
        juce::Rectangle<int> phaseDispersionBounds_;
        bool phaseDispersionOn_ = false;

        juce::Rectangle<int> harmonicEditorBounds_;
        juce::Rectangle<int> footerBounds_;
        juce::Rectangle<int> cpuBarBounds_;
        std::array<float, 16> harmonicHeights_{};
        float footerCpuLoadPercent_ = 0.0f;
        bool embeddedInDesignMode_ = false;
    };

} // namespace pw8::plugin::ui
