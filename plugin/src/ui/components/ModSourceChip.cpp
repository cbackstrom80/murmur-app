#include "ModSourceChip.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr const char* kDragDescriptionPrefix = "pw8modsource:";
    } // namespace

    juce::String modSourceDragDescription(modulation::ModSource source)
    {
        return juce::String(kDragDescriptionPrefix) + juce::String(static_cast<int>(source));
    }

    std::optional<modulation::ModSource> parseModSourceDragDescription(const juce::String& description)
    {
        if (!description.startsWith(kDragDescriptionPrefix))
            return std::nullopt;
        const int ordinal = description.substring(static_cast<int>(std::char_traits<char>::length(kDragDescriptionPrefix))).getIntValue();
        if (ordinal < 0 || ordinal > static_cast<int>(modulation::ModSource::Sidechain))
            return std::nullopt;
        return static_cast<modulation::ModSource>(ordinal);
    }

    juce::String modSourceLabel(modulation::ModSource source)
    {
        using modulation::ModSource;
        switch (source)
        {
            case ModSource::None: return "-";
            case ModSource::Lfo1: return "LFO 1";
            case ModSource::Lfo2: return "LFO 2";
            case ModSource::Lfo3: return "LFO 3";
            case ModSource::Lfo4: return "LFO 4";
            case ModSource::Lfo5: return "LFO 5";
            case ModSource::Lfo6: return "LFO 6";
            case ModSource::Lfo7: return "LFO 7";
            case ModSource::Lfo8: return "LFO 8";
            case ModSource::Env1: return "AMP ENV";
            case ModSource::Env2: return "ENV 2";
            case ModSource::Env3: return "ENV 3";
            case ModSource::Env4: return "ENV 4";
            case ModSource::Env5: return "ENV 5";
            case ModSource::Env6: return "ENV 6";
            case ModSource::Env7: return "ENV 7";
            case ModSource::Env8: return "ENV 8";
            case ModSource::Velocity: return "VELOCITY";
            case ModSource::ChannelPressure: return "CHANNEL PRESSURE";
            case ModSource::PolyAftertouch: return "AFTERTOUCH";
            case ModSource::MpeSlide: return "MPE SLIDE";
            case ModSource::ModWheel: return "MOD WHEEL";
            case ModSource::Expression: return "EXPRESSION";
            case ModSource::Macro1: return "MACRO 1";
            case ModSource::Macro2: return "MACRO 2";
            case ModSource::Macro3: return "MACRO 3";
            case ModSource::Macro4: return "MACRO 4";
            case ModSource::Macro5: return "MACRO 5";
            case ModSource::Macro6: return "MACRO 6";
            case ModSource::Macro7: return "MACRO 7";
            case ModSource::Macro8: return "MACRO 8";
            case ModSource::Sidechain: return "SIDECHAIN";
        }
        return "-";
    }

    juce::String modDestinationLabel(modulation::ModDestination destination, std::uint8_t targetIndex)
    {
        using modulation::ModDestination;
        switch (destination)
        {
            case ModDestination::None: return "-";
            case ModDestination::FilterCutoff: return "GLOBAL FILTER CUTOFF";
            case ModDestination::FilterResonance: return "GLOBAL FILTER RESONANCE";
            case ModDestination::OperatorFilterCutoff:
                return "ENG " + juce::String(static_cast<int>(targetIndex)) + " FILTER CUTOFF";
            case ModDestination::OperatorFilterResonance:
                return "ENG " + juce::String(static_cast<int>(targetIndex)) + " FILTER RES";
            case ModDestination::OperatorLevel: return "OP " + juce::String(static_cast<int>(targetIndex)) + " LEVEL";
            case ModDestination::Pan: return "PAN";
            case ModDestination::OperatorWavetablePosition:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " WT POS";
            case ModDestination::OperatorWavetableBend:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " WT BEND";
            case ModDestination::OperatorWavetableAsymmetry:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " WT ASYM";
            case ModDestination::OperatorWavetableSyncRatio:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " WT SYNC RATIO";
            case ModDestination::OperatorWavetableFormant:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " WT FORMANT";
            case ModDestination::OperatorWavetableSyncAmount:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " WT SYNC AMT";
            case ModDestination::MasterFxMix:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " MIX";
            case ModDestination::MasterReverbMix:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " REV MIX";
            case ModDestination::MasterReverbSize:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " REV SIZE";
            case ModDestination::MasterReverbDecay:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " REV DECAY";
            case ModDestination::MasterReverbPreDelay:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " REV PRE";
            case ModDestination::MasterReverbDiffusion:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " REV DIFF";
            case ModDestination::MasterReverbModDepth:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " REV MOD";
            case ModDestination::MasterGain:
                return "MASTER GAIN";
            case ModDestination::VocoderMix:
                return "FX " + juce::String(static_cast<int>(targetIndex)) + " VOC MIX";
            case ModDestination::VocoderFormant:
                return "FX " + juce::String(static_cast<int>(targetIndex)) + " VOC FORM";
            case ModDestination::ModRouteDepth:
                return "ROUTE " + juce::String(static_cast<int>(targetIndex)) + " DEPTH";
        }
        return "-";
    }

    ModSourceChip::ModSourceChip(ModAssignmentController& controller, modulation::ModSource source,
                                 const juce::String& label, juce::Colour colour)
        : controller_(controller), source_(source), label_(label), colour_(colour)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setHelpText("Click to select, then click a Cutoff/Resonance knob. Or drag onto a ringed knob.");
    }

    void ModSourceChip::paint(juce::Graphics& g)
    {
        const bool armed = controller_.armedSource() == source_;
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        if (armed)
        {
            g.setColour(colour_.withAlpha(0.35f));
            g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
            draw::strokeGlowPath(g, draw::roundedRectPath(bounds, bounds.getHeight() * 0.5f), 0.95f, 1.5f, true);
        }
        else
        {
            draw::fillRecessedRoundedRect(g, bounds, bounds.getHeight() * 0.5f);
            g.setColour(colour_);
            g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() * 0.5f, 1.0f);
        }

        g.setColour(colour_);
        g.fillEllipse(bounds.getX() + 7.0f, bounds.getCentreY() - 3.0f, 6.0f, 6.0f);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        g.drawText(label_, bounds.withTrimmedLeft(18.0f), juce::Justification::centredLeft);
    }

    void ModSourceChip::mouseDown(const juce::MouseEvent&)
    {
        dragStarted_ = false;
    }

    void ModSourceChip::mouseUp(const juce::MouseEvent& event)
    {
        if (dragStarted_ || event.getDistanceFromDragStart() >= 6)
            return;
        controller_.arm(source_);
        repaint();
    }

    void ModSourceChip::mouseDrag(const juce::MouseEvent& event)
    {
        if (dragStarted_ || event.getDistanceFromDragStart() < 6)
            return;

        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            dragStarted_ = true;
            controller_.disarm();
            container->startDragging(modSourceDragDescription(source_), this);
        }
    }

} // namespace pw8::plugin::ui
