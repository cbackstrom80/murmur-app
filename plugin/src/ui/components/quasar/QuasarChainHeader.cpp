#include "QuasarChainHeader.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
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

        void setEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, int typeOrdinal)
        {
            if (auto* param = apvts.getParameter(prefix + "Type"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(typeOrdinal)));
        }
    } // namespace

    QuasarChainHeader::QuasarChainHeader(PatchworkEightProcessor& processor,
                                         juce::AudioProcessorValueTreeState& apvts)
        : processor_(processor), apvts_(apvts), bypassButton_("BYPASS OFF")
    {
        presetLabel_.setFont(fonts::label(11.0f));
        presetLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        presetLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(presetLabel_);

        for (std::size_t i = 0; i < slotPills_.size(); ++i)
        {
            slotPills_[i] = std::make_unique<GlowRingButton>("M" + juce::String(static_cast<int>(i + 1)));
            slotPills_[i]->setClickingTogglesState(false);
            slotPills_[i]->onClick = [this, i] { selectSlot(i + 3); };
            addAndMakeVisible(*slotPills_[i]);
        }

        quasarPill_ = std::make_unique<GlowRingButton>("QUASAR");
        quasarPill_->setAccentColour(juce::Colour(0xff7c4dff));
        quasarPill_->setClickingTogglesState(false);
        quasarPill_->onClick = [this] {
            const auto prefix = slotParamPrefix(slotIndex_);
            const int type = readEffectType(apvts_, prefix);
            setEffectType(apvts_, prefix, type == 13 ? 0 : 13);
            refresh();
            if (onSlotSelected)
                onSlotSelected(slotIndex_);
        };
        addAndMakeVisible(*quasarPill_);

        bypassButton_.setClickingTogglesState(false);
        bypassButton_.onClick = [this] { toggleBypass(); };
        addAndMakeVisible(bypassButton_);

        bindSlot(5);
    }

    juce::String QuasarChainHeader::slotParamPrefix(std::size_t globalFxSlotIndex) const
    {
        return masterFxParamId(masterLocalIndex(globalFxSlotIndex), "");
    }

    std::size_t QuasarChainHeader::masterLocalIndex(std::size_t globalFxSlotIndex) const
    {
        return globalFxSlotIndex >= 3 ? globalFxSlotIndex - 3 : 0;
    }

    void QuasarChainHeader::bindSlot(std::size_t globalFxSlotIndex)
    {
        slotIndex_ = juce::jlimit<std::size_t>(3, 6, globalFxSlotIndex);
        refresh();
    }

    void QuasarChainHeader::selectSlot(std::size_t globalFxSlotIndex)
    {
        slotIndex_ = juce::jlimit<std::size_t>(3, 6, globalFxSlotIndex);
        refresh();
        if (onSlotSelected)
            onSlotSelected(slotIndex_);
    }

    void QuasarChainHeader::toggleBypass()
    {
        const auto prefix = slotParamPrefix(slotIndex_);
        const int type = readEffectType(apvts_, prefix);
        setEffectType(apvts_, prefix, type == 0 ? 13 : 0);
        refresh();
    }

    void QuasarChainHeader::refresh()
    {
        const auto& patch = processor_.getCurrentPatch();
        presetLabel_.setText(patch.metadata.name.empty() ? juce::String("INIT PATCH")
                                                         : juce::String(patch.metadata.name),
                             juce::dontSendNotification);

        for (std::size_t i = 0; i < slotPills_.size(); ++i)
        {
            const auto prefix = masterFxParamId(i, "");
            const int type = readEffectType(apvts_, prefix);
            const bool selected = (i + 3) == slotIndex_;
            slotPills_[i]->setSelectionHighlight(selected);
            slotPills_[i]->setToggleState(type != 0, juce::dontSendNotification);
        }

        const auto activePrefix = slotParamPrefix(slotIndex_);
        const int activeType = readEffectType(apvts_, activePrefix);
        const bool quasarActive = activeType == 13;
        quasarPill_->setSelectionHighlight(quasarActive);
        quasarPill_->setToggleState(quasarActive, juce::dontSendNotification);

        bypassButton_.setToggleState(activeType != 0, juce::dontSendNotification);
        bypassButton_.setButtonText(activeType == 0 ? "BYPASS ON" : "BYPASS OFF");
    }

    void QuasarChainHeader::paint(juce::Graphics& g)
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
    }

    void QuasarChainHeader::resized()
    {
        auto bounds = getLocalBounds().reduced(0, 4);
        presetLabel_.setBounds(bounds.removeFromLeft(juce::jmax(140, bounds.getWidth() / 3)));

        bounds.removeFromLeft(12);
        const int pillW = 44;
        const int gap = 6;
        for (auto& pill : slotPills_)
        {
            pill->setBounds(bounds.removeFromLeft(pillW));
            bounds.removeFromLeft(gap);
        }

        quasarPill_->setBounds(bounds.removeFromLeft(64));
        bounds.removeFromLeft(gap);
        bypassButton_.setBounds(bounds.removeFromRight(96));
    }

} // namespace pw8::plugin::ui
