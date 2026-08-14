#include "DesignFxDetailPanel.h"

#include <cstring>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "FxEffectPlayParams.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] int readEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        [[nodiscard]] bool fieldVisibleForEffectType(const char* suffix, int typeOrdinal) noexcept
        {
            if (suffix == nullptr || suffix[0] == '\0')
                return false;
            if (std::strcmp(suffix, "Type") == 0)
                return false;

            const auto startsWith = [suffix](const char* prefix) {
                return std::strncmp(suffix, prefix, std::strlen(prefix)) == 0;
            };

            if (startsWith("Reverb"))
                return typeOrdinal == 7;
            if (startsWith("Eq"))
                return typeOrdinal == 8;
            if (startsWith("Chorus"))
                return typeOrdinal == 2;

            return std::strcmp(suffix, "Mix") == 0;
        }

        [[nodiscard]] bool isDesignFxDetailSupported(int typeOrdinal) noexcept
        {
            return typeOrdinal == 2 || typeOrdinal == 7 || typeOrdinal == 8;
        }
    } // namespace

    DesignFxDetailPanel::DesignFxDetailPanel(PatchworkEightProcessor& processor) : apvts_(processor.apvts)
    {
        slotHint_.setFont(fonts::label(fonts::kBodyLabelSize));
        slotHint_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(slotHint_);

        for (std::size_t i = 0; i < slotButtons_.size(); ++i)
        {
            auto& btn = slotButtons_[i];
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(9200);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, palette::kAccentDim.withAlpha(0.55f));
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn.setColour(juce::TextButton::textColourOnId, palette::kAccent);
            btn.onClick = [this, i] { selectSlot(i); };
            addAndMakeVisible(btn);
        }

        typeLabel_.setFont(fonts::label(12.0f));
        typeLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(typeLabel_);

        deferredLabel_.setFont(fonts::value(11.0f));
        deferredLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        deferredLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(deferredLabel_);

        addAndMakeVisible(detailFrame_);
        detailFrame_.addAndMakeVisible(knobViewport_);
        knobViewport_.setViewedComponent(&knobContainer_, false);
        knobViewport_.setScrollBarsShown(true, false);

        selectSlot(0);
        startTimerHz(4);
    }

    juce::String DesignFxDetailPanel::slotParamPrefix(std::size_t slotIndex) const
    {
        if (slotIndex < kNumInsertFxSlots)
            return insertFxParamId(slotIndex, "");
        return masterFxParamId(slotIndex - kNumInsertFxSlots, "");
    }

    juce::String DesignFxDetailPanel::slotShortLabel(std::size_t slotIndex) const
    {
        if (slotIndex < kNumInsertFxSlots)
            return "I" + juce::String(static_cast<int>(slotIndex + 1));
        return "M" + juce::String(static_cast<int>(slotIndex - kNumInsertFxSlots + 1));
    }

    void DesignFxDetailPanel::selectSlot(std::size_t index)
    {
        selectedSlot_ = index;
        for (std::size_t i = 0; i < slotButtons_.size(); ++i)
            slotButtons_[i].setToggleState(i == index, juce::dontSendNotification);
        rebuildKnobs();
        resized();
    }

    void DesignFxDetailPanel::rebuildKnobs()
    {
        paramKnobs_.clear();
        knobContainer_.removeAllChildren();

        const auto prefix = slotParamPrefix(selectedSlot_);
        const int type = readEffectType(apvts_, prefix);
        const auto& playSpec = fxPlaySpecForType(type);

        typeLabel_.setText(slotShortLabel(selectedSlot_) + " · " + playSpec.name, juce::dontSendNotification);

        if (!isDesignFxDetailSupported(type))
        {
            deferredLabel_.setText(
                type == 0 ? "Effect bypassed — enable a slot in PLAY FX or pick Reverb/EQ/Chorus."
                          : "Full detail for this effect type lands in a later sprint (MVP: Reverb, EQ, Chorus).",
                juce::dontSendNotification);
            deferredLabel_.setVisible(true);
            knobViewport_.setVisible(false);
            return;
        }

        deferredLabel_.setVisible(false);
        knobViewport_.setVisible(true);

        for (const auto& spec : kEffectSlotFieldSpecs)
        {
            if (!fieldVisibleForEffectType(spec.idSuffix, type))
                continue;

            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + spec.idSuffix, spec.label);
            knobContainer_.addAndMakeVisible(*knob);
            paramKnobs_.push_back(std::move(knob));
        }

        resized();
    }

    void DesignFxDetailPanel::timerCallback()
    {
        const auto prefix = slotParamPrefix(selectedSlot_);
        const int type = readEffectType(apvts_, prefix);
        const auto& playSpec = fxPlaySpecForType(type);
        typeLabel_.setText(slotShortLabel(selectedSlot_) + " · " + playSpec.name, juce::dontSendNotification);

        if (paramKnobs_.empty() && isDesignFxDetailSupported(type))
            rebuildKnobs();
    }

    void DesignFxDetailPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(8);

        auto slotRow = bounds.removeFromTop(28);
        slotHint_.setBounds(slotRow.removeFromLeft(42));
        const int slotWidth = slotRow.getWidth() / static_cast<int>(slotButtons_.size());
        for (auto& btn : slotButtons_)
            btn.setBounds(slotRow.removeFromLeft(slotWidth).reduced(2, 2));

        bounds.removeFromTop(6);
        typeLabel_.setBounds(bounds.removeFromTop(20));
        bounds.removeFromTop(4);

        if (deferredLabel_.isVisible())
        {
            deferredLabel_.setBounds(bounds);
            detailFrame_.setVisible(false);
            return;
        }

        detailFrame_.setVisible(true);
        detailFrame_.setBounds(bounds);
        knobViewport_.setBounds(detailFrame_.getContentBounds());

        constexpr int kKnobWidth = 72;
        constexpr int kKnobHeight = 88;
        constexpr int kKnobGap = 8;
        const int columns = juce::jmax(1, knobViewport_.getWidth() / (kKnobWidth + kKnobGap));
        const int rows = static_cast<int>((paramKnobs_.size() + static_cast<std::size_t>(columns) - 1) /
                                          static_cast<std::size_t>(columns));
        const int contentHeight = juce::jmax(knobViewport_.getHeight(), rows * (kKnobHeight + kKnobGap) + kKnobGap);
        knobContainer_.setSize(knobViewport_.getWidth(), contentHeight);

        int x = kKnobGap;
        int y = kKnobGap;
        int col = 0;
        for (const auto& knob : paramKnobs_)
        {
            knob->setBounds(x, y, kKnobWidth, kKnobHeight);
            ++col;
            if (col >= columns)
            {
                col = 0;
                x = kKnobGap;
                y += kKnobHeight + kKnobGap;
            }
            else
            {
                x += kKnobWidth + kKnobGap;
            }
        }
    }

} // namespace pw8::plugin::ui
