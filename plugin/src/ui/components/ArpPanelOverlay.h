#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ArpStepStrip.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    /// Full-screen arpeggiator lab — Figma `murmur-arp-view` (4:1267).
    class ArpPanelOverlay : public juce::Component, private juce::Timer
    {
    public:
        explicit ArpPanelOverlay(PatchworkEightProcessor& processor);
        ~ArpPanelOverlay() override;

        std::function<void()> onClosed;

        void showDrawer();
        void dismiss();

        /// When embedded under `DesignModeEditor`, back navigates to design ENGINE sub-page.
        void setEmbeddedInDesignMode(bool embedded);

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void timerCallback() override;
        void refreshEngineRouting();
        void setArpMode(int modeOrdinal);
        void setOctaveRange(int octaves);
        void setSyncDivisionQuick(int buttonIndex);
        [[nodiscard]] juce::String engineRoutingSubtitle(int engineIndex) const;
        void paintVelocityCurve(juce::Graphics& g, juce::Rectangle<int> area) const;
        void paintHoldRow(juce::Graphics& g) const;
        void paintFooterBar(juce::Graphics& g) const;
        void paintFooterModChips(juce::Graphics& g) const;
        [[nodiscard]] bool modSourceActive(modulation::ModSource source) const;

        PatchworkEightProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;

        juce::TextButton backButton_{"← PLAY BOARD"};
        juce::Label badgeLabel_;
        juce::Label titleLabel_;
        juce::Label syncReadoutLabel_;
        GlowRingButton enableButton_{"ARP ON"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment_;

        SectionPanel leftPanel_;
        juce::Label playbackDirectionLabel_;
        std::array<juce::TextButton, 7> modeButtons_{};
        juce::Label octaveRangeLabel_;
        std::array<juce::TextButton, 4> octaveButtons_{};
        juce::Label holdLabel_;
        GlowRingButton latchButton_{};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> latchAttachment_;
        juce::Rectangle<int> holdRowBounds_;

        SectionPanel stepPanel_;
        juce::Viewport stepViewport_;
        ArpStepStrip stepStrip_;

        SectionPanel timingPanel_;
        juce::Label clockResolutionLabel_;
        std::array<juce::TextButton, 4> syncButtons_{};
        juce::Slider gateSlider_;
        juce::Label gateLabel_;
        std::unique_ptr<GlowKnob> numStepsKnob_;
        std::unique_ptr<GlowKnob> swingKnob_;
        std::unique_ptr<GlowKnob> rateHzKnob_;

        SectionPanel curvePanel_;
        juce::Label curveTypeLabel_;
        juce::Label curveCaption_;

        SectionPanel routingPanel_;
        std::array<juce::Label, 8> routeEngineLabels_;
        std::array<juce::Label, 8> routeSubLabels_;
        std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 8> mixAttachments_;
        std::array<std::unique_ptr<GlowRingButton>, 8> mixButtons_;

        juce::Label footerCpuLabel_;
        juce::Label footerCpuPercentLabel_;
        juce::Label footerVoicesLabel_;
        juce::Label footerVoicesCountLabel_;
        juce::Label footerMidiLabel_;
        juce::Label footerModSourcesLabel_;
        std::array<juce::Rectangle<int>, 5> footerModChipBounds_{};
        juce::Rectangle<int> footerBounds_;
        juce::Rectangle<int> cpuBarBounds_;
        float footerCpuLoadPercent_ = 0.0f;
        juce::TextButton panicButton_{"PANIC RESET"};
        bool embeddedInDesignMode_ = false;
    };

} // namespace pw8::plugin::ui
