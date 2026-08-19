#include "DesignUtilityPeaksPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float fallback)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }
    } // namespace

    DesignUtilityPeaksPanel::DesignUtilityPeaksPanel(MurmurProcessor& processor) : processor_(processor)
    {
        titleLabel_.setText("UTILITY PEAKS", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("Mini envelope + mini LFO utility cards (no drums)", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(10.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        footerHint_.setText("Enable a slot, pick ENV or LFO, then route PEAKS A/B in the MOD matrix", juce::dontSendNotification);
        footerHint_.setFont(fonts::micro(9.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        for (std::size_t slot = 0; slot < slots_.size(); ++slot)
        {
            auto& ui = slots_[slot];
            ui.panel = std::make_unique<SectionPanel>(slot == 0 ? "Utility A" : "Utility B", palette::kAccent);
            addAndMakeVisible(*ui.panel);

            ui.enableButton = std::make_unique<GlowRingButton>("Enable");
            ui.enableButton->setClickingTogglesState(true);
            ui.panel->addAndMakeVisible(*ui.enableButton);
            ui.enabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                processor_.apvts, peaksUtilitySlotParamId(slot, "Enabled"), *ui.enableButton);

            ui.envModeButton_.onClick = [this, slot] { setSlotMode(slot, 0); };
            ui.lfoModeButton_.onClick = [this, slot] { setSlotMode(slot, 1); };
            ui.panel->addAndMakeVisible(ui.envModeButton_);
            ui.panel->addAndMakeVisible(ui.lfoModeButton_);

            ui.envKnob_ = std::make_unique<ConcentricGlowKnob>(
                processor_.apvts, peaksUtilitySlotParamId(slot, "AttackMs"), peaksUtilitySlotParamId(slot, "ReleaseMs"),
                "ATK", "REL");
            ui.envKnob_->setInnerAccentColour(palette::kAccentWarm);
            ui.envKnob_->applyFigmaContext(figma::KnobContext::UtilityPeaks);

            ui.lfoKnob_ = std::make_unique<ConcentricGlowKnob>(
                processor_.apvts, peaksUtilitySlotParamId(slot, "LfoDepth"), peaksUtilitySlotParamId(slot, "LfoRateHz"),
                "DEPTH", "RATE");
            ui.lfoKnob_->setInnerAccentColour(palette::kModLfo);
            ui.lfoKnob_->applyFigmaContext(figma::KnobContext::UtilityPeaks);

            for (auto* k : {ui.envKnob_.get(), ui.lfoKnob_.get()})
                ui.panel->addAndMakeVisible(*k);

            ui.triggerButton_.onClick = [this, slot] { processor_.triggerPeaksUtilitySlot(slot); };
            ui.panel->addAndMakeVisible(ui.triggerButton_);

            refreshModeButtons(slot);
        }

        startTimerHz(24);
    }

    DesignUtilityPeaksPanel::~DesignUtilityPeaksPanel() { stopTimer(); }

    void DesignUtilityPeaksPanel::setSlotMode(std::size_t slotIndex, int modeIndex)
    {
        if (auto* param = processor_.apvts.getParameter(peaksUtilitySlotParamId(slotIndex, "Mode")))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(juce::jlimit(0, 1, modeIndex))));
        refreshModeButtons(slotIndex);
    }

    void DesignUtilityPeaksPanel::refreshModeButtons(std::size_t slotIndex)
    {
        auto& ui = slots_[slotIndex];
        const int mode = static_cast<int>(
            loadParam(processor_.apvts, peaksUtilitySlotParamId(slotIndex, "Mode"), 0.0f) + 0.5f);
        const bool env = mode == 0;
        ui.envModeButton_.setColour(juce::TextButton::buttonColourId,
                                    env ? palette::kAccent.withAlpha(0.22f) : juce::Colour(0xff12141a));
        ui.envModeButton_.setColour(juce::TextButton::textColourOffId, env ? palette::kAccent : palette::kTextDim);
        ui.lfoModeButton_.setColour(juce::TextButton::buttonColourId,
                                    !env ? palette::kModLfo.withAlpha(0.22f) : juce::Colour(0xff12141a));
        ui.lfoModeButton_.setColour(juce::TextButton::textColourOffId, !env ? palette::kModLfo : palette::kTextDim);
        ui.envKnob_->setVisible(env);
        ui.lfoKnob_->setVisible(!env);
        ui.triggerButton_.setVisible(env);
    }

    void DesignUtilityPeaksPanel::showOverlay() { setVisible(true); }

    void DesignUtilityPeaksPanel::dismiss() { setVisible(false); }

    void DesignUtilityPeaksPanel::setEmbeddedInDesignMode(bool embedded)
    {
        embeddedInDesignMode_ = embedded;
        backButton_.setVisible(!embedded);
    }

    void DesignUtilityPeaksPanel::refreshFromPatch()
    {
        for (std::size_t i = 0; i < slots_.size(); ++i)
            refreshModeButtons(i);
    }

    void DesignUtilityPeaksPanel::timerCallback()
    {
        const auto levels = processor_.getPeaksUtilityLevels();
        vizLevels_ = levels;
        for (std::size_t i = 0; i < vizBounds_.size(); ++i)
            repaint(vizBounds_[i]);
    }

    void DesignUtilityPeaksPanel::paintSlotViz(juce::Graphics& g, juce::Rectangle<int> bounds, std::size_t slotIndex) const
    {
        if (bounds.isEmpty())
            return;

        draw::fillRecessedRoundedRect(g, bounds.toFloat(), 4.0f);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::micro(8.0f));
        g.drawText(slotIndex == 0 ? "PEAK A" : "PEAK B", bounds.reduced(8, 4).removeFromTop(10),
                   juce::Justification::centredLeft);

        const float level = juce::jlimit(0.0f, 1.0f, std::abs(vizLevels_[slotIndex]));
        auto fill = bounds.reduced(6, 14).toFloat();
        fill.setHeight(fill.getHeight() * level);
        fill.setY(bounds.getBottom() - 8 - fill.getHeight());
        g.setColour(slotIndex == 0 ? palette::kAccent.withAlpha(0.55f) : palette::kModLfo.withAlpha(0.55f));
        g.fillRoundedRectangle(fill, 3.0f);

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::micro(8.5f));
        g.drawText(juce::String(static_cast<int>(level * 100.0f)) + "%",
                   bounds.reduced(6).removeFromBottom(12), juce::Justification::centredRight);
    }

    void DesignUtilityPeaksPanel::paint(juce::Graphics& g) { g.fillAll(palette::kBackgroundTop); }

    void DesignUtilityPeaksPanel::paintOverChildren(juce::Graphics& g)
    {
        for (std::size_t i = 0; i < vizBounds_.size(); ++i)
            paintSlotViz(g, vizBounds_[i], i);
    }

    void DesignUtilityPeaksPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignUtilityPeaksPageOuterMargin);
        auto header = bounds.removeFromTop(layout::kDesignUtilityPeaksHeaderHeight);
        backButton_.setBounds(header.removeFromLeft(90).reduced(0, 8));
        titleLabel_.setBounds(header.removeFromTop(18));
        subtitleLabel_.setBounds(header.removeFromTop(14));

        footerHint_.setBounds(getLocalBounds().removeFromBottom(layout::kDesignUtilityPeaksFooterHeight)
                                  .reduced(layout::kDesignUtilityPeaksPageOuterMargin, 8));
        bounds.removeFromBottom(layout::kDesignUtilityPeaksFooterHeight);

        const int cardW = (bounds.getWidth() - layout::kDesignUtilityPeaksPageSectionGap) / 2;
        const int cardH = bounds.getHeight();
        const int knobW = layout::kDesignUtilityPeaksKnobSize + 18;
        const int knobH = layout::kDesignUtilityPeaksKnobRowHeight;

        for (std::size_t slot = 0; slot < slots_.size(); ++slot)
        {
            juce::Rectangle<int> card;
            if (slot == 0)
                card = bounds.removeFromLeft(cardW);
            else
            {
                bounds.removeFromLeft(layout::kDesignUtilityPeaksPageSectionGap);
                card = bounds.removeFromLeft(cardW);
            }
            card = card.withHeight(cardH);

            auto& ui = slots_[slot];
            ui.panel->setBounds(card);

            auto inner = getLocalArea(ui.panel.get(), ui.panel->getContentBounds()).reduced(8, 4);
            if (inner.isEmpty())
                inner = card.reduced(12, 28);

            auto top = inner.removeFromTop(24);
            ui.enableButton->setBounds(top.removeFromLeft(72));
            top.removeFromLeft(6);
            ui.envModeButton_.setBounds(top.removeFromLeft(44).reduced(2));
            ui.lfoModeButton_.setBounds(top.removeFromLeft(44).reduced(2));

            inner.removeFromTop(8);
            vizBounds_[slot] = inner.removeFromTop(layout::kDesignUtilityPeaksVizHeight).reduced(4, 2);
            inner.removeFromTop(8);

            auto knobRow = inner.removeFromTop(knobH);
            ui.envKnob_->setBounds(knobRow.removeFromLeft(knobW).withHeight(knobH));
            ui.triggerButton_.setBounds(knobRow.removeFromLeft(52).withHeight(knobH).reduced(2, 12));
            ui.lfoKnob_->setBounds(knobRow.removeFromLeft(knobW).withHeight(knobH));
        }
    }

    bool DesignUtilityPeaksPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
