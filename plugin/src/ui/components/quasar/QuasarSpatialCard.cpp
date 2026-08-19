#include "QuasarSpatialCard.h"

#include "../../theme/FigmaKnobTokens.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        struct RoomPreset
        {
            const char* label;
            float roomAmount;
            float roomSize;
        };

        static constexpr RoomPreset kRoomPresets[] = {
            {"INTIMATE", 0.22f, 0.75f},
            {"STAGE", 0.45f, 1.0f},
            {"VOID", 0.62f, 1.35f},
        };
    } // namespace

    QuasarSpatialCard::QuasarSpatialCard(MurmurProcessor& processor,
                                         juce::AudioProcessorValueTreeState& apvts)
        : processor_(processor), apvts_(apvts), panel_("SPATIAL", juce::Colour(0xffe040fb)), hpCompButton_("HP COMP OFF")
    {
        addAndMakeVisible(panel_);

        roomHeader_.setText("ROOM PRESET", juce::dontSendNotification);
        roomHeader_.setFont(fonts::label(7.0f));
        roomHeader_.setColour(juce::Label::textColourId, palette::kTextDim);
        roomHeader_.setJustificationType(juce::Justification::centredLeft);
        panel_.addAndMakeVisible(roomHeader_);

        for (std::size_t i = 0; i < roomPills_.size(); ++i)
        {
            stylePill(roomPills_[i], kRoomPresets[i].label);
            roomPills_[i].onClick = [this, amount = kRoomPresets[i].roomAmount, size = kRoomPresets[i].roomSize] {
                applyRoomPreset(amount, size);
                refresh();
            };
            panel_.addAndMakeVisible(roomPills_[i]);
        }

        hpCompButton_.setClickingTogglesState(false);
        hpCompButton_.onClick = [this] {
            const float current = readParam("QuasarCrossfeed", 0.0f);
            setParam("QuasarCrossfeed", current > 0.01f ? 0.0f : 0.35f);
            refresh();
        };
        panel_.addAndMakeVisible(hpCompButton_);

        bindSlot(5);
    }

    juce::String QuasarSpatialCard::slotParamPrefix() const
    {
        const std::size_t local = slotIndex_ >= 3 ? slotIndex_ - 3 : 0;
        return masterFxParamId(local, "");
    }

    float QuasarSpatialCard::readParam(const juce::String& suffix, float fallback) const
    {
        if (auto* raw = apvts_.getRawParameterValue(slotParamPrefix() + suffix))
            return raw->load();
        return fallback;
    }

    void QuasarSpatialCard::setParam(const juce::String& suffix, float value)
    {
        if (auto* param = apvts_.getParameter(slotParamPrefix() + suffix))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    }

    void QuasarSpatialCard::applyRoomPreset(float roomAmount, float roomSize)
    {
        setParam("Qsr1RoomAmount", roomAmount);
        setParam("Qsr1RoomSize", roomSize);
        setParam("Qsr2RoomAmount", roomAmount * 0.92f);
        setParam("Qsr2RoomSize", roomSize * 1.05f);
    }

    void QuasarSpatialCard::stylePill(juce::TextButton& btn, const juce::String& text)
    {
        btn.setButtonText(text);
        btn.setClickingTogglesState(false);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
    }

    void QuasarSpatialCard::highlightPill(juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId,
                      active ? juce::Colour(0xffe040fb).withAlpha(0.12f) : juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, active ? juce::Colour(0xffe040fb) : palette::kTextDim);
    }

    void QuasarSpatialCard::bindSlot(std::size_t globalFxSlotIndex)
    {
        slotIndex_ = juce::jlimit<std::size_t>(3, 6, globalFxSlotIndex);
        airKnob_.reset();
        airKnob_ = std::make_unique<GlowKnob>(apvts_, slotParamPrefix() + "Qsr1Level", "AIR");
        airKnob_->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        panel_.addAndMakeVisible(*airKnob_);
        refresh();
        resized();
    }

    void QuasarSpatialCard::refresh()
    {
        const float amount = readParam("Qsr1RoomAmount", 0.45f);
        const float size = readParam("Qsr1RoomSize", 1.0f);
        for (std::size_t i = 0; i < roomPills_.size(); ++i)
        {
            const bool active = std::abs(amount - kRoomPresets[i].roomAmount) < 0.05f
                                && std::abs(size - kRoomPresets[i].roomSize) < 0.08f;
            highlightPill(roomPills_[i], active);
        }

        const bool hpComp = readParam("QuasarCrossfeed", 0.0f) > 0.01f;
        hpCompButton_.setToggleState(hpComp, juce::dontSendNotification);
        hpCompButton_.setButtonText(hpComp ? "HP COMP ON" : "HP COMP OFF");
    }

    void QuasarSpatialCard::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds().reduced(8, 6);

        auto roomRow = content.removeFromTop(52);
        roomHeader_.setBounds(roomRow.removeFromTop(12));
        const int chipW = juce::jmax(64, (roomRow.getWidth() - 8) / 3);
        for (auto& pill : roomPills_)
        {
            pill.setBounds(roomRow.removeFromLeft(chipW).reduced(0, 2));
            roomRow.removeFromLeft(4);
        }

        content.removeFromTop(8);
        auto bottom = content;
        if (airKnob_ != nullptr)
            airKnob_->setBounds(bottom.removeFromLeft(88).removeFromTop(88));
        bottom.removeFromLeft(12);
        hpCompButton_.setBounds(bottom.removeFromTop(28).removeFromLeft(110));
    }

} // namespace pw8::plugin::ui
