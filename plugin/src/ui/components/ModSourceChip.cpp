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
        if (ordinal < 0 || ordinal > static_cast<int>(modulation::ModSource::RandomX))
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
            case ModSource::Random1: return "RND 1";
            case ModSource::Random2: return "RND 2";
            case ModSource::Random3: return "RND 3";
            case ModSource::Random4: return "RND 4";
            case ModSource::RandomT: return "RND T";
            case ModSource::RandomX: return "RND X";
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
            case ModDestination::FilterModeMorph: return "GLOBAL FILTER MODE MORPH";
            case ModDestination::FilterRouting: return "FILTER ROUTING MORPH";
            case ModDestination::FilterDrive: return "FILTER 2 DRIVE";
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
            case ModDestination::MorphPosition:
                return "MORPH POSITION";
            case ModDestination::MasterDynamicsMix:
                return "MASTER DYNAMICS MIX";
            case ModDestination::SidechainDepth:
                return "SIDECHAIN DEPTH";
            case ModDestination::QuasarQsr1Angle:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR1 AZ";
            case ModDestination::QuasarQsr2Angle:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR2 AZ";
            case ModDestination::QuasarRoomAmount:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR ROOM";
            case ModDestination::QuasarCrossfeed:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR XFEED";
            case ModDestination::QuasarDelayVolume:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR DLY";
            case ModDestination::QuasarQsr1Distance:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR1 DIST";
            case ModDestination::QuasarQsr2Distance:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR2 DIST";
            case ModDestination::QuasarDelayTime:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR DLY T";
            case ModDestination::QuasarDelayFeedback:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR FDBK";
            case ModDestination::QuasarQsr1Height:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR1 HT";
            case ModDestination::QuasarQsr2Height:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR2 HT";
            case ModDestination::QuasarCntrLevel:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR CNTR";
            case ModDestination::QuasarQsr1Level:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR1 LVL";
            case ModDestination::QuasarQsr2Level:
                return "MASTER " + juce::String(static_cast<int>(targetIndex)) + " QSR2 LVL";
            case ModDestination::OperatorFmModulatorRatio:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " FM RATIO";
            case ModDestination::OperatorFmModulatorIndex:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " FM INDEX";
            case ModDestination::OperatorFmModulatorFeedback:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " FM FDBK";
            case ModDestination::OperatorFreqRatio:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " FREQ RATIO";
            case ModDestination::OperatorPhaseBend:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " PHASE BEND";
            case ModDestination::OperatorPhaseFold:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " PHASE FOLD";
            case ModDestination::OperatorPhaseAsymmetry:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " PHASE ASYM";
            case ModDestination::OperatorAdditivePartialCount:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " PARTIALS";
            case ModDestination::OperatorAdditiveTilt:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " ADD TILT";
            case ModDestination::OperatorAdditiveOddEven:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " ODD/EVEN";
            case ModDestination::OperatorAdditiveStretch:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " STRETCH";
            case ModDestination::OperatorResonatorStructure:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " RES STR";
            case ModDestination::OperatorResonatorDecay:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " RES DEC";
            case ModDestination::OperatorResonatorDamping:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " RES DAMP";
            case ModDestination::OperatorResonatorBrightness:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " RES BRIGHT";
            case ModDestination::OperatorResonatorModeCount:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " RES MODES";
            case ModDestination::OperatorGrainDensity:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " GRAIN DENS";
            case ModDestination::OperatorGrainSizeMs:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " GRAIN SIZE";
            case ModDestination::OperatorGrainPositionJitter:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " POS JIT";
            case ModDestination::OperatorGrainPitchJitter:
                return "OP " + juce::String(static_cast<int>(targetIndex)) + " PITCH JIT";
            case ModDestination::UnisonVoices: return "UNISON VOICES";
            case ModDestination::UnisonDetune: return "UNISON DETUNE";
            case ModDestination::UnisonSpread: return "UNISON SPREAD";
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
