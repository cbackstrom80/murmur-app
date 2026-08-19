#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PlayModeLayout.h"
#include "GlowKnob.h"
#include "processor/MurmurProcessor.h"
#include "quasar/QuasarBinauralFieldView.h"
#include "quasar/QuasarChainHeader.h"
#include "quasar/QuasarEngineCard.h"
#include "quasar/QuasarPrimaryKnobRow.h"
#include "quasar/QuasarSegmentedMeter.h"
#include "quasar/QuasarSpatialCard.h"
#include "quasar/QuasarTelemetryBar.h"

namespace pw8::plugin::ui
{
    /// Sprint 8 Q1 — Figma `murmur-master-quasar-binaural` (`102:4`).
    class MasterQuasarPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit MasterQuasarPanel(MurmurProcessor& processor);
        ~MasterQuasarPanel() override;

        std::function<void()> onClosed;
        std::function<void()> onOpenFxChain;

        void setEmbeddedInDesignMode(bool embedded);
        void showForFxSlot(std::size_t slotIndex);
        void dismiss();

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        class ScrollContent;

        void timerCallback() override;
        void bindSlot(std::size_t slotIndex);
        [[nodiscard]] juce::String slotParamPrefix(std::size_t slotIndex) const;
        [[nodiscard]] juce::File resolveCompanionQuasarFile() const;
        void importCompanionQuasar();

        MurmurProcessor& processor_;
        juce::AudioProcessorValueTreeState& apvts_;
        std::size_t slotIndex_ = 5;
        bool embeddedInDesignMode_ = false;

        juce::TextButton backButton_{"← PLAY BOARD"};
        juce::Label specBadge_;
        juce::Label titleLabel_;
        juce::Label subtitleLabel_;
        juce::TextButton importQuasarButton_{"IMPORT .QUASAR"};
        juce::TextButton openFxChainButton_{"OPEN FULL FX CHAIN"};

        QuasarChainHeader chainHeader_;
        QuasarSegmentedMeter segmentedMeter_;
        std::unique_ptr<QuasarBinauralFieldView> fieldView_;
        QuasarPrimaryKnobRow primaryKnobRow_;
        std::unique_ptr<GlowKnob> orbitMacroKnob_;
        std::unique_ptr<GlowKnob> spreadMacroKnob_;
        QuasarEngineCard engineCard_;
        QuasarSpatialCard spatialCard_;
        QuasarTelemetryBar telemetryBar_;

        juce::Viewport viewport_;
        ScrollContent* scrollContent_ = nullptr;
    };

} // namespace pw8::plugin::ui
