#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ConcentricGlowKnob.h"
#include "GlowKnob.h"
#include "SectionPanel.h"
#include "../PlayModeLayout.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/LfoWireframeView.h"

namespace pw8::plugin::ui
{
    /// Full-screen dual-LFO lab — Figma `murmur-dual-lfo-lab` (15:247).
    class DualLfoLabPanel : public juce::Component
    {
    public:
        explicit DualLfoLabPanel(PatchworkEightProcessor& processor);
        ~DualLfoLabPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenModMatrix;

        void showOverlay();
        void dismiss();

        void setEmbeddedInDesignMode(bool embedded);

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        struct LfoColumn
        {
            SectionPanel panel;
            wireframe::LfoWireframeView wireframe;
            std::unique_ptr<GlowKnob> waveform;
            std::unique_ptr<GlowKnob> mode;
            std::unique_ptr<ConcentricGlowKnob> motion;
            juce::Label routingLabel;
            juce::Label routingLegend;
            juce::Label syncHeaderLabel;
            std::array<juce::TextButton, 7> syncButtons{};
            std::size_t lfoIndex = 0;

            LfoColumn(PatchworkEightProcessor& processor, std::size_t lfoIndex, const char* title, juce::Colour accent);
            void rebind(PatchworkEightProcessor& processor, std::size_t newIndex, const char* title, juce::Colour accent);
            void layoutColumn();
        };

        void bindPairTab(int tabIndex);
        void selectFooterLfo(std::size_t lfoIndex);
        void highlightPairTabs();
        void highlightFooterChips();
        void highlightSyncButtons(LfoColumn& column);
        void layoutColumns();

        PatchworkEightProcessor& processor_;
        juce::TextButton backButton_{"← PLAY BOARD"};
        juce::Label badgeLabel_;
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::Label footerHint_;
        juce::TextButton openModMatrixButton_{"OPEN MOD MATRIX"};
        std::array<juce::TextButton, 3> pairTabButtons_{};
        std::array<juce::TextButton, layout::kDesignDualLfoFooterChipCount> footerChipButtons_{};

        LfoColumn lfo1_;
        LfoColumn lfo2_;
        LfoColumn lfo3_;
        LfoColumn lfo4_;
        int activePairTab_ = 0;
        std::size_t activeFooterLfo_ = 0;
        juce::Rectangle<int> footerBounds_;
        bool embeddedInDesignMode_ = false;
    };

} // namespace pw8::plugin::ui
