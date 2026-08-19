#include "QuasarPrimaryKnobRow.h"

#include "../../PlayModeLayout.h"
#include "../../theme/FigmaKnobTokens.h"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        struct KnobSpec
        {
            const char* field;
            const char* label;
            modulation::ModDestination destination;
            bool enableMod;
        };

        static constexpr KnobSpec kKnobSpecs[] = {
            {"CntrLevel", "WIDTH", modulation::ModDestination::QuasarCntrLevel, true},
            {"QuasarCrossfeed", "CROSSFEED", modulation::ModDestination::QuasarCrossfeed, true},
            {"Qsr1Distance", "DISTANCE", modulation::ModDestination::QuasarQsr1Distance, true},
            {"Qsr1AngleDeg", "AZIMUTH", modulation::ModDestination::QuasarQsr1Angle, true},
            {"Qsr1Height", "ELEVATION", modulation::ModDestination::QuasarQsr1Height, true},
            {"Qsr1RoomSize", "HRTF SIZE", modulation::ModDestination::QuasarRoomAmount, false},
            {"Mix", "MIX", modulation::ModDestination::MasterFxMix, true},
        };
    } // namespace

    QuasarPrimaryKnobRow::QuasarPrimaryKnobRow(PatchworkEightProcessor& processor,
                                               juce::AudioProcessorValueTreeState& apvts)
        : processor_(processor), apvts_(apvts)
    {
        rebuild(5);
    }

    std::size_t QuasarPrimaryKnobRow::masterLocalIndex(std::size_t globalFxSlotIndex) const
    {
        return globalFxSlotIndex >= 3 ? globalFxSlotIndex - 3 : 0;
    }

    void QuasarPrimaryKnobRow::rebuild(std::size_t globalFxSlotIndex)
    {
        knobs_.clear();
        const auto prefix = masterFxParamId(masterLocalIndex(globalFxSlotIndex), "");
        const auto targetIndex = static_cast<std::uint8_t>(masterLocalIndex(globalFxSlotIndex));

        for (const auto& spec : kKnobSpecs)
        {
            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + spec.field, spec.label);
            knob->applyFigmaContext(figma::KnobContext::DesignVocoder);
            if (spec.enableMod)
                knob->enableModulationTarget(processor_, spec.destination, targetIndex);
            addAndMakeVisible(*knob);
            knobs_.push_back(std::move(knob));
        }

        resized();
    }

    void QuasarPrimaryKnobRow::resized()
    {
        auto bounds = getLocalBounds();
        const int knobW = layout::kQuasarPrimaryKnobCellWidth;
        const int gap = 10;
        for (auto& knob : knobs_)
        {
            if (bounds.getWidth() < knobW)
                break;
            knob->setMaxDialDiameter(layout::kQuasarPrimaryKnobDialSize);
            knob->setBounds(bounds.removeFromLeft(knobW).withHeight(getHeight()));
            bounds.removeFromLeft(gap);
        }
    }

} // namespace pw8::plugin::ui
