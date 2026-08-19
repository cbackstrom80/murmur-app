#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ConcentricGlowKnob.h"
#include "GlowKnob.h"
#include "GlowRingButton.h"
#include "ModAssignmentController.h"
#include "SectionPanel.h"
#include "processor/MurmurProcessor.h"
#include "wireframe/FilterRoutingWireframeView.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-mi-ui-design-filter-lab` (`89:313`) — Blades dual-filter design lab (Track B-M4).
    class DesignFilterLabPanel : public juce::Component, private juce::Timer
    {
    public:
        DesignFilterLabPanel(MurmurProcessor& processor, ModAssignmentController& assignmentController);
        ~DesignFilterLabPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenModMatrix;
        std::function<void()> onOpenPlayFilter;

        void showOverlay();
        void dismiss();
        void setEmbeddedInDesignMode(bool embedded);
        void refreshFromPatch();

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        class ModRouteListModel;

        void timerCallback() override;
        void paintPanelCard(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintF1Spectrum(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintF2DriveMeter(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void styleModePill(juce::TextButton& btn, bool active);
        void refreshModePills();
        void rebuildModRouteList();

        MurmurProcessor& processor_;
        ModAssignmentController& assignmentController_;
        bool embeddedInDesignMode_ = false;

        juce::TextButton backButton_{"← DESIGN"};
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;

        wireframe::FilterRoutingWireframeView routingWireframe_;
        juce::Rectangle<int> f1SpectrumBounds_;
        juce::Rectangle<int> f2DriveBounds_;

        SectionPanel f1Panel_{"Filter 1 — State Variable"};
        SectionPanel f2Panel_{"Filter 2 — Ladder Drive"};
        SectionPanel routingHeroPanel_{"Signal Pipeline"};

        GlowRingButton f1EnabledButton_{"F1 Enable"};
        GlowRingButton f2EnabledButton_{"F2 Enable"};
        juce::Label f1ActiveBadge_;
        juce::Label f2WarmthBadge_;
        std::array<juce::TextButton, 4> f1ModePills_{};
        std::unique_ptr<GlowKnob> f1ModeMorphKnob_;
        std::unique_ptr<ConcentricGlowKnob> f1ToneKnob_;
        std::unique_ptr<GlowKnob> f1KeyTrackKnob_;

        std::unique_ptr<GlowKnob> f2CutoffOffsetKnob_;
        std::unique_ptr<GlowKnob> f2ResonanceKnob_;
        std::unique_ptr<GlowKnob> f2DriveKnob_;
        std::unique_ptr<GlowKnob> f2KeyTrackKnob_;

        std::unique_ptr<GlowKnob> routingMorphKnob_;
        juce::Label routingMorphLabel_;
        juce::Label routingMorphValueLabel_;

        SectionPanel modRoutesPanel_{"Active Mod Routes"};
        juce::ListBox modRouteList_;
        std::unique_ptr<ModRouteListModel> modRouteModel_;
        std::vector<modulation::ModRoute> filterModRoutes_;

        juce::TextButton openModMatrixButton_{"OPEN MOD MATRIX"};
        juce::TextButton openPlayFilterButton_{"OPEN IN PLAY FILTER"};
        juce::Label footerHint_;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> f1EnabledAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> f2EnabledAttachment_;
    };

} // namespace pw8::plugin::ui
