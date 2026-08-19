#include "EngineOscillatorPicker.h"

#include <cmath>
#include <cstring>

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "EngineIconGrid.h"
#include "state/PluginState.h"
#include "wireframe/WavetableMeshPaint.h"
#include "wireframe/WireframeProjection.h"

namespace pw8::plugin::ui
{
    namespace
    {
        using layout::kDesignModeV2CardRowGap;
        using layout::kDesignModeV2ContextVisualizerHeight;
        using layout::kDesignModeV2SubPickerArrowWidth;
        using layout::kDesignModeV2SubPickerHeight;
        using layout::kDesignModeV2SubPickerPillHeight;
        using layout::kDesignModeV2TypeStripHeight;
        using layout::kEngineOscCellCornerRadius;
        using layout::kEngineOscCellGap;
        using layout::kEngineOscCellHeight;
        using layout::kEngineOscCellWidth;
        using layout::kEngineOscContextGridHeight;
        using layout::kEngineOscStripGridGap;
        using layout::kEngineOscStripHeight;
        using layout::kEngineOscTypePillFontSize;

        [[nodiscard]] const char* typePillLabel(algorithm::EngineType engine) noexcept
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic: return "CLS";
                case algorithm::EngineType::Wavetable: return "WT";
                case algorithm::EngineType::FmPm: return "FM";
                case algorithm::EngineType::Additive: return "ADD";
                case algorithm::EngineType::PhaseShape: return "PHS";
                case algorithm::EngineType::Granular: return "GRN";
                case algorithm::EngineType::NoiseChaos: return "NSE";
                case algorithm::EngineType::Resonator: return "RES";
                case algorithm::EngineType::External: return "EXT";
            }
            return "?";
        }

        [[nodiscard]] bool loadBool(juce::AudioProcessorValueTreeState& apvts, const juce::String& id) noexcept
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load() >= 0.5f;
            return false;
        }

        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                      float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        [[nodiscard]] int waveformToSubPickerIndex(int waveformOrdinal) noexcept
        {
            switch (waveformOrdinal)
            {
                case 0: return 0; // SIN
                case 2: return 1; // SAW
                case 1: return 2; // TRI
                case 3: return 3; // SQR/PLS
                default: return 0;
            }
        }

        [[nodiscard]] int subPickerIndexToWaveform(int pillIndex) noexcept
        {
            static constexpr int kMap[] = {0, 2, 1, 3, 3};
            return kMap[juce::jlimit(0, 4, pillIndex)];
        }

        [[nodiscard]] oscillator::WtWarpParams loadWavetableWarpParams(juce::AudioProcessorValueTreeState& apvts,
                                                                       std::size_t opIndex)
        {
            oscillator::WtWarpParams warpParams;
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtBend")))
                warpParams.bend = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtAsymmetry")))
                warpParams.asymmetry = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtSyncRatio")))
                warpParams.syncRatio = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtSyncAmount")))
                warpParams.syncAmount = raw->load();
            return warpParams;
        }
    } // namespace

    EngineOscillatorPicker::EngineOscillatorPicker(MurmurProcessor& processor, int engineIndex)
        : processor_(processor), engineIndex_(engineIndex), contextThumb_(processor, engineIndex)
    {
        addChildComponent(contextThumb_);
        setWantsKeyboardFocus(true);

        startTimerHz(30);
        refreshPreviews();
    }

    algorithm::EngineType EngineOscillatorPicker::currentEngineType() const
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (auto* raw = processor_.apvts.getRawParameterValue(operatorParamId(idx, "Engine")))
            return static_cast<algorithm::EngineType>(static_cast<int>(raw->load() + 0.5f));
        return algorithm::EngineType::Classic;
    }

    int EngineOscillatorPicker::numTypePills() const { return engineIndex_ == 0 ? 9 : 8; }

    algorithm::EngineType EngineOscillatorPicker::engineForPill(int pillIndex) const
    {
        const int maxOrdinal = engineIndex_ == 0 ? static_cast<int>(algorithm::EngineType::External)
                                               : static_cast<int>(algorithm::EngineType::Resonator);
        return static_cast<algorithm::EngineType>(juce::jlimit(0, maxOrdinal, pillIndex));
    }

    bool EngineOscillatorPicker::isEngineLive() const
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (!loadBool(processor_.apvts, operatorMixParamId(idx, "MixEnabled")))
            return false;
        if (loadBool(processor_.apvts, operatorMixParamId(idx, "MixMute")))
            return false;

        for (std::size_t op = 0; op < kNumOperators; ++op)
        {
            if (loadBool(processor_.apvts, operatorMixParamId(op, "MixSolo")))
            {
                if (!loadBool(processor_.apvts, operatorMixParamId(idx, "MixSolo")))
                    return false;
                break;
            }
        }
        return true;
    }

    void EngineOscillatorPicker::refreshPreviews()
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        const auto engine = currentEngineType();

        if (engine == algorithm::EngineType::Wavetable || engine == algorithm::EngineType::Granular)
            processor_.ensureOperatorWavetableLoaded(op);

        for (int w = 0; w < 4; ++w)
        {
            wireframe::ClassicPreviewParams cp;
            cp.waveform = wireframe::OscPreviewSampler::waveformFromOrdinal(w);
            cp.pulseWidth = loadParam(processor_.apvts, operatorParamId(op, "PulseWidth"), 0.5f);
            wireframe::OscPreviewSampler::sampleClassicCycle(cp, classicPreviews_[static_cast<std::size_t>(w)]);

            wireframe::FmPreviewParams fp;
            fp.carrier = cp;
            fp.modulator.waveform = wireframe::OscPreviewSampler::waveformFromOrdinal(w);
            fp.modulator.pulseWidth = 0.5f;
            fp.modRatio = 1.0f + static_cast<float>(w) * 0.5f;
            fp.modIndex = 0.4f + static_cast<float>(w) * 0.35f;
            wireframe::OscPreviewSampler::sampleFmLayers(fp, fmCarrierPreviews_[static_cast<std::size_t>(w)],
                                                         fmModPreviews_[static_cast<std::size_t>(w)]);

            wireframe::PhaseShapePreviewParams pp;
            pp.phaseShape = static_cast<float>(w) / 3.0f;
            pp.phaseBend = 0.15f * static_cast<float>(w - 1);
            wireframe::OscPreviewSampler::samplePhaseShapeCycle(pp, phaseOutPreviews_[static_cast<std::size_t>(w)],
                                                                phaseOutPreviews_[static_cast<std::size_t>(w)]);
        }

        if (engine == algorithm::EngineType::Additive || engine == algorithm::EngineType::Resonator)
        {
            if (engine == algorithm::EngineType::Additive)
            {
                wireframe::AdditivePreviewParams ap;
                ap.partialCount = static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "AdditivePartialCount"), 32.0f));
                ap.tilt = loadParam(processor_.apvts, operatorParamId(op, "AdditiveTilt"));
                wireframe::OscPreviewSampler::computeAdditiveHeights(ap, additiveHeights_, additiveBarCount_);
            }
            else
            {
                wireframe::ResonatorPreviewParams rp;
                rp.modeCount = static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "ResonatorModeCount"), 6.0f));
                wireframe::OscPreviewSampler::computeResonatorHeights(rp, resonatorHeights_, resonatorBarCount_);
            }
        }

        for (int w = 0; w < 4; ++w)
        {
            wireframe::NoisePreviewParams np;
            np.variant = wireframe::OscPreviewSampler::noiseVariantFromOrdinal(w);
            np.seed = static_cast<std::uint64_t>(engineIndex_) + static_cast<std::uint64_t>(w + 1);
            std::array<float, wireframe::kNoisePoints> full{};
            wireframe::OscPreviewSampler::sampleNoiseTrace(np, full);
            for (int i = 0; i < wireframe::kPreviewPoints; ++i)
            {
                const int src = i * wireframe::kNoisePoints / wireframe::kPreviewPoints;
                noisePreviews_[static_cast<std::size_t>(w)][static_cast<std::size_t>(i)] = full[static_cast<std::size_t>(src)];
            }
        }

        {
            wireframe::FmPreviewParams liveFp;
            liveFp.carrier.waveform = wireframe::OscPreviewSampler::waveformFromOrdinal(
                static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "Waveform"))));
            liveFp.modulator.waveform = wireframe::OscPreviewSampler::waveformFromOrdinal(
                static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "FmModulatorWaveform"))));
            liveFp.modulator.pulseWidth = loadParam(processor_.apvts, operatorParamId(op, "PulseWidth"), 0.5f);
            liveFp.modRatio = loadParam(processor_.apvts, operatorParamId(op, "FmModulatorRatio"), 1.0f);
            liveFp.modIndex = loadParam(processor_.apvts, operatorParamId(op, "FmModulatorIndex"), 1.0f);
            wireframe::OscPreviewSampler::sampleFmLayers(liveFp, fmLiveCarrier_, fmLiveMod_);
        }

        switch (engine)
        {
            case algorithm::EngineType::Classic:
                activeContextCell_ = juce::jlimit(0, 3, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "Waveform"))));
                activeSubPickerIndex_ = waveformToSubPickerIndex(activeContextCell_);
                break;
            case algorithm::EngineType::Wavetable:
            case algorithm::EngineType::Granular:
            {
                const float pos = loadParam(processor_.apvts, operatorParamId(op, "WavetablePos"));
                activeContextCell_ = juce::jlimit(0, 3, static_cast<int>(pos * 3.0f + 0.5f));
                activeSubPickerIndex_ = engine == algorithm::EngineType::Wavetable
                                            ? juce::jlimit(0, 2, static_cast<int>(
                                                                  loadParam(processor_.apvts, operatorParamId(op, "WtMorphMode"))
                                                                      + 0.5f))
                                            : 0;
                break;
            }
            case algorithm::EngineType::FmPm:
                activeContextCell_ = juce::jlimit(
                    0, 3, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "FmModulatorWaveform"))));
                activeSubPickerIndex_ = juce::jlimit(0, 7, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "FmModulatorWaveform"))));
                break;
            case algorithm::EngineType::Additive:
                activeSubPickerIndex_ = juce::jlimit(0, 1, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "AdditiveOddEven")) * 2.0f));
                break;
            case algorithm::EngineType::PhaseShape:
                activeSubPickerIndex_ = juce::jlimit(0, 2, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "PhaseShape")) * 2.0f + 0.5f));
                break;
            case algorithm::EngineType::NoiseChaos:
                activeContextCell_ =
                    juce::jlimit(0, 3, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "NoiseVariant"))));
                activeSubPickerIndex_ = juce::jlimit(0, 4, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "NoiseVariant"))));
                break;
            case algorithm::EngineType::Resonator:
                activeSubPickerIndex_ = juce::jlimit(0, 6, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "ResonatorStructure")) * 6.0f + 0.5f));
                break;
            case algorithm::EngineType::External:
                activeSubPickerIndex_ = juce::jlimit(
                    0, 3, static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "ExternalInputSource"))));
                break;
            default:
                break;
        }

        syncContextThumb();
    }

    void EngineOscillatorPicker::syncContextThumb()
    {
        EngineOscContextPreviewData data;
        data.engineType = currentEngineType();
        data.engineLive = engineLive_;
        data.animPhase = animPhase_;
        data.motionGain = motionGain_;
        data.activeSubPickerIndex = activeSubPickerIndex_;
        data.classicPreviews = &classicPreviews_;
        data.fmLiveCarrier = &fmLiveCarrier_;
        data.fmLiveMod = &fmLiveMod_;
        data.phaseOutPreviews = &phaseOutPreviews_;
        data.additiveHeights = &additiveHeights_;
        data.additiveBarCount = additiveBarCount_;
        data.resonatorHeights = &resonatorHeights_;
        data.resonatorBarCount = resonatorBarCount_;
        contextThumb_.setPreviewData(data);
    }

    void EngineOscillatorPicker::advanceAnimation()
    {
        engineLive_ = isEngineLive();
        if (!engineLive_)
        {
            motionGain_ = 1.0f;
            return;
        }

        const auto idx = static_cast<std::size_t>(engineIndex_);
        float ratio = loadParam(processor_.apvts, operatorParamId(idx, "FreqRatio"), 1.0f);
        ratio = juce::jlimit(0.25f, 8.0f, ratio);
        animPhase_ += 0.014f * ratio;
        if (animPhase_ >= 1.0f)
            animPhase_ -= 1.0f;

        std::array<float, 256> scope{};
        const int pulled = processor_.readScopeSamples(scope.data(), static_cast<int>(scope.size()));
        if (pulled > 8)
        {
            double sumSq = 0.0;
            for (int i = 0; i < pulled; ++i)
                sumSq += static_cast<double>(scope[static_cast<std::size_t>(i)])
                         * static_cast<double>(scope[static_cast<std::size_t>(i)]);
            motionGain_ = juce::jlimit(0.82f, 1.18f, 0.88f + static_cast<float>(std::sqrt(sumSq / static_cast<double>(pulled))) * 2.4f);
        }
        else
            motionGain_ = 0.92f;

        syncContextThumb();
    }

    float EngineOscillatorPicker::previewSample(const std::array<float, wireframe::kPreviewPoints>& buf, float t,
                                                 float phaseOffset, float amp) const
    {
        float u = t + phaseOffset;
        u -= std::floor(u);
        const float f = u * static_cast<float>(wireframe::kPreviewPoints - 1);
        const int i0 = juce::jlimit(0, wireframe::kPreviewPoints - 1, static_cast<int>(f));
        const int i1 = (i0 + 1) % wireframe::kPreviewPoints;
        const float frac = f - static_cast<float>(i0);
        return juce::jlimit(-1.2f, 1.2f,
                            (buf[static_cast<std::size_t>(i0)] * (1.0f - frac) + buf[static_cast<std::size_t>(i1)] * frac) * amp);
    }

    void EngineOscillatorPicker::timerCallback()
    {
        if (++previewRefreshCounter_ >= 3)
        {
            previewRefreshCounter_ = 0;
            refreshPreviews();
        }
        advanceAnimation();
        repaint();
    }

    void EngineOscillatorPicker::setEngineType(algorithm::EngineType type)
    {
        if (!algorithm::isEngineImplemented(type))
            return;
        if (!algorithm::isExternalEngineAllowed(pw8::core::NodeId{static_cast<std::uint8_t>(engineIndex_)}, type))
            return;

        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (auto* param = processor_.apvts.getParameter(operatorParamId(idx, "Engine")))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(static_cast<int>(type))));
        processor_.syncCurrentPatchFromApvts();
        if (type == algorithm::EngineType::Wavetable || type == algorithm::EngineType::Granular)
            processor_.ensureOperatorWavetableLoaded(idx);
        processor_.notifyPatchMetadataChanged();
        refreshPreviews();
        if (designModeV2Layout_)
            resized();
        repaint();
    }

    void EngineOscillatorPicker::activateContextCell(int cellIndex)
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        auto& apvts = processor_.apvts;

        auto setParam = [&](const char* id, float value) {
            if (auto* param = apvts.getParameter(operatorParamId(op, id)))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        switch (currentEngineType())
        {
            case algorithm::EngineType::Classic:
                setParam("Waveform", static_cast<float>(cellIndex));
                break;
            case algorithm::EngineType::Wavetable:
            case algorithm::EngineType::Granular:
                setParam("WavetablePos", static_cast<float>(cellIndex) / 3.0f);
                break;
            case algorithm::EngineType::FmPm:
                if (cellIndex == 0)
                {
                    setParam("Waveform", 0.0f);
                    setParam("FmModulatorWaveform", 0.0f);
                    setParam("FmModulatorIndex", 0.5f);
                }
                else if (cellIndex == 1)
                {
                    setParam("Waveform", 2.0f);
                    setParam("FmModulatorWaveform", 3.0f);
                    setParam("FmModulatorIndex", 0.8f);
                }
                else if (cellIndex == 2)
                {
                    setParam("FmModulatorFeedback", 0.85f);
                    setParam("FmModulatorIndex", 0.6f);
                }
                else
                {
                    setParam("FmModulatorIndex", 1.5f);
                    setParam("FmModulatorRatio", 2.0f);
                }
                break;
            case algorithm::EngineType::Additive:
                setParam("AdditivePartialCount", static_cast<float>(8 << cellIndex));
                break;
            case algorithm::EngineType::PhaseShape:
                setParam("PhaseShape", static_cast<float>(cellIndex) / 3.0f);
                break;
            case algorithm::EngineType::NoiseChaos:
                setParam("NoiseVariant", static_cast<float>(cellIndex));
                break;
            case algorithm::EngineType::Resonator:
                setParam("ResonatorModeCount", static_cast<float>(2 + cellIndex * 2));
                break;
            default:
                break;
        }

        activeContextCell_ = cellIndex;
        refreshPreviews();
        repaint();
    }

    int EngineOscillatorPicker::pillIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < static_cast<int>(typePillLayout_.size()); ++i)
        {
            if (typePillLayout_[static_cast<std::size_t>(i)].contains(pos))
                return i;
        }
        return -1;
    }

    int EngineOscillatorPicker::cellIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if (cellLayout_[static_cast<std::size_t>(i)].contains(pos))
                return i;
        }
        return -1;
    }

    bool EngineOscillatorPicker::usesWavetablePositionControls() const
    {
        const auto engine = currentEngineType();
        return engine == algorithm::EngineType::Wavetable || engine == algorithm::EngineType::Granular;
    }

    void EngineOscillatorPicker::setWavetablePositionNormalized(float pos01)
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        if (auto* param = processor_.apvts.getParameter(operatorParamId(op, "WavetablePos")))
        {
            const float next = juce::jlimit(0.0f, 1.0f, pos01);
            param->setValueNotifyingHost(next);
            activeContextCell_ = juce::jlimit(0, 3, static_cast<int>(next * 3.0f + 0.5f));
        }
        refreshPreviews();
        repaint();
    }

    void EngineOscillatorPicker::stepWavetablePosition(int delta)
    {
        if (delta == 0)
            return;

        const auto op = static_cast<std::size_t>(engineIndex_);
        if (auto* param = processor_.apvts.getParameter(operatorParamId(op, "WavetablePos")))
        {
            const float step = currentEngineType() == algorithm::EngineType::Granular ? (1.0f / 31.0f)
                                                                                      : (1.0f / 255.0f);
            const float next =
                juce::jlimit(0.0f, 1.0f, param->getValue() + static_cast<float>(delta) * step);
            param->setValueNotifyingHost(next);
            activeContextCell_ = juce::jlimit(0, 3, static_cast<int>(next * 3.0f + 0.5f));
        }

        refreshPreviews();
        repaint();
    }

    void EngineOscillatorPicker::loadGranularSampleFromFile()
    {
        if (currentEngineType() != algorithm::EngineType::Granular)
            return;

        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Load a wavetable JSON for granular playback...", juce::File(), "*.json");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        const juce::Component::SafePointer<EngineOscillatorPicker> safeThis(this);
        const int nodeAtRequestTime = engineIndex_;
        fileChooser_->launchAsync(flags, [safeThis, nodeAtRequestTime](const juce::FileChooser& chooser) {
            auto* self = safeThis.getComponent();
            if (self == nullptr)
                return;

            const auto file = chooser.getResult();
            if (!file.existsAsFile())
                return;

            self->processor_.setOperatorWavetableFile(static_cast<std::size_t>(nodeAtRequestTime),
                                                      file.getFullPathName());
            self->refreshPreviews();
            self->repaint();
        });
    }

    void EngineOscillatorPicker::paintContextPositionArrows(juce::Graphics& g)
    {
        if (!designModeV2Layout_ || !contextUsesPositionArrows_)
            return;

        auto paintArrow = [&](juce::Rectangle<int> bounds, bool left) {
            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
            g.setColour(palette::kBorder.withAlpha(0.65f));
            g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 3.0f, 1.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(9.0f));
            g.drawText(left ? juce::String(juce::CharPointer_UTF8("\xe2\x97\x80"))
                            : juce::String(juce::CharPointer_UTF8("\xe2\x96\xb6")),
                       bounds, juce::Justification::centred);
        };

        if (!contextLeftArrow_.isEmpty())
            paintArrow(contextLeftArrow_, true);
        if (!contextRightArrow_.isEmpty())
            paintArrow(contextRightArrow_, false);
    }

    bool EngineOscillatorPicker::keyPressed(const juce::KeyPress& key)
    {
        if (!designModeV2Layout_ || !usesWavetablePositionControls())
            return false;

        if (key == juce::KeyPress::leftKey || key == juce::KeyPress::upKey)
        {
            stepWavetablePosition(-1);
            return true;
        }
        if (key == juce::KeyPress::rightKey || key == juce::KeyPress::downKey)
        {
            stepWavetablePosition(1);
            return true;
        }

        return false;
    }

    void EngineOscillatorPicker::mouseDown(const juce::MouseEvent& event)
    {
        grabKeyboardFocus();

        if (const int pill = pillIndexAt(event.getPosition()); pill >= 0)
        {
            setEngineType(engineForPill(pill));
            return;
        }

        if (designModeV2Layout_)
        {
            if (subPickerUsesCycler_)
            {
                if (subPickerLeftArrow_.contains(event.getPosition()))
                {
                    stepSubPickerCycler(-1);
                    return;
                }
                if (subPickerRightArrow_.contains(event.getPosition()))
                {
                    stepSubPickerCycler(1);
                    return;
                }
            }
            else if (const int subPill = subPickerPillIndexAt(event.getPosition()); subPill >= 0)
            {
                activateSubPickerPill(subPill);
                return;
            }

            if (contextUsesPositionArrows_)
            {
                if (contextLeftArrow_.contains(event.getPosition()))
                {
                    stepWavetablePosition(-1);
                    return;
                }
                if (contextRightArrow_.contains(event.getPosition()))
                {
                    stepWavetablePosition(1);
                    return;
                }
                const auto thumbBounds = contextThumb_.getBounds();
                if (thumbBounds.contains(event.getPosition()))
                {
                    const float norm =
                        static_cast<float>(event.getPosition().x - thumbBounds.getX())
                        / static_cast<float>(juce::jmax(1, thumbBounds.getWidth()));
                    setWavetablePositionNormalized(norm);
                    return;
                }
            }
        }

        if (const int cell = cellIndexAt(event.getPosition()); cell >= 0)
            activateContextCell(cell);
    }

    void EngineOscillatorPicker::mouseDoubleClick(const juce::MouseEvent& event)
    {
        if (!designModeV2Layout_)
            return;

        const auto engine = currentEngineType();
        if (engine != algorithm::EngineType::Wavetable && engine != algorithm::EngineType::Granular)
            return;

        if (!contextPreviewBounds_.contains(event.getPosition()))
            return;

        if (onWavetableLabRequested)
            onWavetableLabRequested();
    }

    void EngineOscillatorPicker::paintWaveCell(juce::Graphics& g, juce::Rectangle<float> cell, int cellIndex,
                                               bool selected, const std::array<float, wireframe::kPreviewPoints>& samples,
                                               const char* label)
    {
        const float cellPhase = engineLive_ ? animPhase_ * (selected ? 1.0f : 0.35f) : 0.0f;
        const float cellAmp = engineLive_ ? (selected ? motionGain_ : 0.78f) : 1.0f;

        draw::fillRecessedRoundedRect(g, cell, static_cast<float>(kEngineOscCellCornerRadius));
        g.setColour(selected ? palette::kAccent.withAlpha(0.22f) : palette::kPanel.withAlpha(0.45f));
        g.fillRoundedRectangle(cell, static_cast<float>(kEngineOscCellCornerRadius));
        if (selected)
            draw::strokeGlowPath(g, draw::roundedRectPath(cell.reduced(0.5f), static_cast<float>(kEngineOscCellCornerRadius)),
                                 engineLive_ ? 1.0f : 0.95f, 1.4f, true);
        else
        {
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(cell, static_cast<float>(kEngineOscCellCornerRadius), 1.0f);
        }

        wireframe::paintFlatWaveform(
            g, cell.reduced(4.0f, 7.0f), wireframe::kPreviewPoints,
            [&](float t) { return previewSample(samples, t, cellPhase, cellAmp); }, selected && engineLive_);

        g.setColour(selected ? palette::kAccent : palette::kTextDim);
        g.setFont(juce::Font(juce::FontOptions(7.0f)));
        g.drawText(label, cell.withTrimmedTop(cell.getHeight() - 9.0f), juce::Justification::centred);
        juce::ignoreUnused(cellIndex);
    }

    void EngineOscillatorPicker::paintWavetableWaveCell(juce::Graphics& g, juce::Rectangle<float> cell, int cellIndex,
                                                        bool selected, float framePos01, const char* label,
                                                        bool granularOverlay)
    {
        draw::fillRecessedRoundedRect(g, cell, static_cast<float>(kEngineOscCellCornerRadius));
        g.setColour(selected ? palette::kAccent.withAlpha(0.22f) : palette::kPanel.withAlpha(0.45f));
        g.fillRoundedRectangle(cell, static_cast<float>(kEngineOscCellCornerRadius));
        if (selected)
            draw::strokeGlowPath(g, draw::roundedRectPath(cell.reduced(0.5f), static_cast<float>(kEngineOscCellCornerRadius)),
                                 engineLive_ ? 1.0f : 0.95f, 1.4f, true);
        else
        {
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(cell, static_cast<float>(kEngineOscCellCornerRadius), 1.0f);
        }

        auto waveArea = cell.reduced(4.0f, 7.0f);
        const auto idx = static_cast<std::size_t>(engineIndex_);
        if (const auto* table = processor_.getActiveWavetableTable(idx); table != nullptr && table->isValid())
        {
            const auto warpParams = loadWavetableWarpParams(processor_.apvts, idx);
            wireframe::paintWavetableFrameWaveform(g, waveArea, table, framePos01, warpParams,
                                                   selected && engineLive_, wireframe::kPreviewPoints);
            if (granularOverlay)
            {
                wireframe::GranularOverlayParams grainParams;
                grainParams.wavetablePos = framePos01;
                grainParams.grainSizeMs =
                    loadParam(processor_.apvts, operatorParamId(idx, "GrainSizeMs"), 60.0f);
                wireframe::paintGranularGrainOverlay(g, waveArea, grainParams);
            }
        }
        else
        {
            wireframe::paintFlatWaveform(
                g, waveArea, wireframe::kPreviewPoints,
                [&](float t) {
                    return previewSample(classicPreviews_[static_cast<std::size_t>(juce::jmin(cellIndex, 3))], t,
                                         engineLive_ ? animPhase_ * 0.35f : 0.0f, 1.0f);
                },
                selected && engineLive_);
        }

        g.setColour(selected ? palette::kAccent : palette::kTextDim);
        g.setFont(juce::Font(juce::FontOptions(7.0f)));
        g.drawText(label, cell.withTrimmedTop(cell.getHeight() - 9.0f), juce::Justification::centred);
    }

    void EngineOscillatorPicker::paintBarCell(juce::Graphics& g, juce::Rectangle<float> cell, bool selected,
                                              const std::function<float(int)>& heightAt, int barCount)
    {
        draw::fillRecessedRoundedRect(g, cell, static_cast<float>(kEngineOscCellCornerRadius));
        g.setColour(selected ? palette::kAccent.withAlpha(0.22f) : palette::kPanel.withAlpha(0.45f));
        g.fillRoundedRectangle(cell, static_cast<float>(kEngineOscCellCornerRadius));
        wireframe::paintBarLandscape(g, cell.reduced(3.0f), barCount, heightAt);
    }

    void EngineOscillatorPicker::paintTypeStrip(juce::Graphics& g)
    {
        if (typePillLayout_.empty())
            return;

        const auto current = currentEngineType();

        if (designModeV2Layout_)
        {
            auto strip = typePillLayout_.front();
            strip = strip.getUnion(typePillLayout_.back());
            const auto stripBounds = strip.toFloat();

            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(stripBounds, 3.0f);
            g.setColour(palette::kBorder.withAlpha(0.65f));
            g.drawRoundedRectangle(stripBounds.reduced(0.5f), 3.0f, 1.0f);

            for (int i = 0; i < static_cast<int>(typePillLayout_.size()); ++i)
            {
                const auto engine = engineForPill(i);
                const bool selected = engine == current;
                const bool implemented = algorithm::isEngineImplemented(engine);
                const auto bounds = typePillLayout_[static_cast<std::size_t>(i)].toFloat().reduced(0.5f, 1.0f);

                if (selected)
                {
                    g.setColour(palette::kFigmaTeal.withAlpha(0.22f));
                    g.fillRoundedRectangle(bounds, 2.0f);
                    g.setColour(palette::kFigmaTeal.withAlpha(0.85f));
                    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);
                }

                if (i > 0)
                {
                    const float x = bounds.getX();
                    g.setColour(palette::kBorder.withAlpha(0.45f));
                    g.drawVerticalLine(static_cast<int>(x), stripBounds.getY() + 2.0f, stripBounds.getBottom() - 2.0f);
                }

                g.setColour(!implemented ? palette::kFigmaTextDim.withAlpha(0.45f)
                                       : (selected ? palette::kFigmaTextPrimary : palette::kFigmaTextDim));
                g.setFont(fonts::label(8.0f));
                g.drawText(typePillLabel(engine), bounds.toNearestInt(), juce::Justification::centred);
            }
            return;
        }

        for (int i = 0; i < static_cast<int>(typePillLayout_.size()); ++i)
        {
            const auto engine = engineForPill(i);
            const bool selected = engine == current;
            const bool implemented = algorithm::isEngineImplemented(engine);
            const auto bounds = typePillLayout_[static_cast<std::size_t>(i)].toFloat();

            g.setColour(selected ? palette::kAccentDim : palette::kPanel);
            g.fillRoundedRectangle(bounds, 3.0f);
            g.setColour(selected ? palette::kAccent : palette::kBorder.withAlpha(0.7f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

            g.setColour(!implemented ? palette::kTextDim : (selected ? palette::kTextPrimary : palette::kTextSecondary));
            g.setFont(juce::Font(juce::FontOptions(static_cast<float>(kEngineOscTypePillFontSize))));
            g.drawText(typePillLabel(engine), bounds, juce::Justification::centred);
        }
    }

    void EngineOscillatorPicker::paintContextGrid(juce::Graphics& g)
    {
        const auto engine = currentEngineType();
        if (engine == algorithm::EngineType::External)
        {
            const auto area = cellLayout_[0].toFloat().getUnion(cellLayout_[3].toFloat());
            g.setColour(palette::kMurmurViolet.withAlpha(0.85f));
            g.setFont(fonts::label(8.0f));
            g.drawText("EXT\nSIDECHAIN", area, juce::Justification::centred);
            return;
        }

        static constexpr const char* kClassicLabels[] = {"SIN", "TRI", "SAW", "SQR"};
        static constexpr const char* kWtLabels[] = {"WT1", "WT2", "WT3", "WT4"};

        for (int i = 0; i < 4; ++i)
        {
            const auto cell = cellLayout_[static_cast<std::size_t>(i)].toFloat().reduced(1.0f);
            const bool selected = i == activeContextCell_;

            switch (engine)
            {
                case algorithm::EngineType::Classic:
                    paintWaveCell(g, cell, i, selected, classicPreviews_[static_cast<std::size_t>(i)], kClassicLabels[i]);
                    break;
                case algorithm::EngineType::Wavetable:
                case algorithm::EngineType::Granular:
                {
                    const float framePos = static_cast<float>(i) / 3.0f;
                    paintWavetableWaveCell(g, cell, i, selected, framePos, kWtLabels[i],
                                           engine == algorithm::EngineType::Granular);
                    break;
                }
                case algorithm::EngineType::FmPm:
                    paintWaveCell(g, cell, i, selected, fmCarrierPreviews_[static_cast<std::size_t>(i)],
                                  i == 0 ? "CARR" : i == 1 ? "BRIT" : i == 2 ? "COMB" : "BLOB");
                    break;
                case algorithm::EngineType::PhaseShape:
                    paintWaveCell(g, cell, i, selected, phaseOutPreviews_[static_cast<std::size_t>(i)], "SHP");
                    break;
                case algorithm::EngineType::Additive:
                    paintBarCell(g, cell, selected,
                                 [&](int b) { return additiveHeights_[static_cast<std::size_t>(b % additiveBarCount_)]; },
                                 juce::jmin(6, additiveBarCount_));
                    break;
                case algorithm::EngineType::Resonator:
                    paintBarCell(g, cell, selected,
                                 [&](int b) { return resonatorHeights_[static_cast<std::size_t>(b % resonatorBarCount_)]; },
                                 juce::jmin(6, resonatorBarCount_));
                    break;
                case algorithm::EngineType::NoiseChaos:
                    paintWaveCell(g, cell, i, selected, noisePreviews_[static_cast<std::size_t>(i)],
                                  i == 0 ? "WHT" : i == 1 ? "PNK" : i == 2 ? "BRN" : "BLU");
                    break;
                default:
                    paintWaveCell(g, cell, i, selected, classicPreviews_[static_cast<std::size_t>(i)], "...");
                    break;
            }
        }
    }

    void EngineOscillatorPicker::paint(juce::Graphics& g)
    {
        if (playBoardCompactMode_)
        {
            paintPlayBoardStub(g);
            return;
        }

        if (designModeV2Layout_)
        {
            if (!contextPreviewBounds_.isEmpty())
            {
                draw::fillRecessedRoundedRect(g, contextPreviewBounds_.toFloat(), 6.0f);
                g.setColour(palette::kBorder.withAlpha(0.55f));
                g.drawRoundedRectangle(contextPreviewBounds_.toFloat().reduced(0.5f), 6.0f, 1.0f);
            }
            paintTypeStrip(g);
            paintSubPicker(g);
            paintContextPositionArrows(g);
            return;
        }

        paintTypeStrip(g);
        paintContextGrid(g);
    }

    void EngineOscillatorPicker::setPlayBoardCompactMode(bool compact)
    {
        if (playBoardCompactMode_ == compact)
            return;

        playBoardCompactMode_ = compact;
        if (compact)
        {
            designModeV2Layout_ = false;
            contextThumb_.setVisible(false);
        }
        resized();
        repaint();
    }

    void EngineOscillatorPicker::setDesignModeV2Layout(bool designMode)
    {
        if (designModeV2Layout_ == designMode)
            return;

        designModeV2Layout_ = designMode;
        if (designMode)
            playBoardCompactMode_ = false;

        contextThumb_.setSkipChrome(designMode);
        contextThumb_.setVisible(designMode);
        resized();
        repaint();
    }

    void EngineOscillatorPicker::paintSubPickerPills(juce::Graphics& g, juce::Rectangle<int> area,
                                                     const std::vector<const char*>& labels, int activeIndex)
    {
        subPillLayout_.clear();
        if (labels.empty())
            return;

        const int pillH = kDesignModeV2SubPickerPillHeight;
        const int pillY = area.getY() + (area.getHeight() - pillH) / 2;
        int totalW = 0;
        std::vector<int> pillWidths;
        pillWidths.reserve(labels.size());
        for (const auto* label : labels)
        {
            const int w = juce::jmax(27, static_cast<int>(std::strlen(label)) * 6 + 14);
            pillWidths.push_back(w);
            totalW += w;
        }
        totalW += static_cast<int>(labels.size() - 1) * 3;

        int pillX = area.getX() + juce::jmax(0, (area.getWidth() - totalW) / 2);

        for (std::size_t i = 0; i < labels.size(); ++i)
        {
            const auto bounds = juce::Rectangle<int>(pillX, pillY, pillWidths[i], pillH);
            subPillLayout_.push_back(bounds);

            const bool selected = static_cast<int>(i) == activeIndex;
            const bool design = designModeV2Layout_;
            g.setColour(selected ? (design ? palette::kFigmaTeal.withAlpha(0.22f) : palette::kAccentDim) : palette::kPanel);
            g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
            g.setColour(selected ? (design ? palette::kFigmaTeal.withAlpha(0.85f) : palette::kAccent)
                                 : palette::kBorder.withAlpha(0.7f));
            g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 3.0f, 1.0f);
            g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
            g.setFont(juce::Font(juce::FontOptions(7.0f)));
            g.drawText(labels[i], bounds, juce::Justification::centred);

            pillX += pillWidths[static_cast<std::size_t>(i)] + 3;
        }
    }

    void EngineOscillatorPicker::paintSubPickerLabeled(juce::Graphics& g, juce::Rectangle<int> area, const char* label,
                                                       const std::vector<const char*>& labels, int activeIndex)
    {
        auto labelArea = area.removeFromLeft(juce::jmin(56, area.getWidth() / 3));
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText(label, labelArea, juce::Justification::centredLeft);
        paintSubPickerPills(g, area, labels, activeIndex);
    }

    void EngineOscillatorPicker::paintSubPickerSingleButton(juce::Graphics& g, juce::Rectangle<int> area,
                                                            const char* label, const char* buttonText)
    {
        subPillLayout_.clear();
        auto labelArea = area.removeFromLeft(juce::jmin(48, area.getWidth() / 3));
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText(label, labelArea, juce::Justification::centredLeft);

        const int pillH = kDesignModeV2SubPickerPillHeight;
        const int pillW = juce::jmax(52, static_cast<int>(std::strlen(buttonText)) * 6 + 16);
        const auto bounds = juce::Rectangle<int>(area.getX(), area.getY() + (area.getHeight() - pillH) / 2, pillW, pillH);
        subPillLayout_.push_back(bounds);

        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
        g.setColour(palette::kBorder.withAlpha(0.7f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 3.0f, 1.0f);
        g.setColour(palette::kTextSecondary);
        g.setFont(juce::Font(juce::FontOptions(7.0f)));
        g.drawText(buttonText, bounds, juce::Justification::centred);
    }

    void EngineOscillatorPicker::paintSubPickerCycler(juce::Graphics& g, juce::Rectangle<int> area,
                                                      const juce::String& title, const juce::String& indexText)
    {
        subPickerLeftArrow_ = area.removeFromLeft(kDesignModeV2SubPickerArrowWidth).withSizeKeepingCentre(
            kDesignModeV2SubPickerArrowWidth, 19);
        subPickerRightArrow_ = area.removeFromRight(kDesignModeV2SubPickerArrowWidth).withSizeKeepingCentre(
            kDesignModeV2SubPickerArrowWidth, 19);

        auto paintArrow = [&](juce::Rectangle<int> bounds, bool left) {
            g.setColour(palette::kPanelRaised);
            g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
            g.setColour(palette::kBorder.withAlpha(0.65f));
            g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 3.0f, 1.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(9.0f));
            g.drawText(left ? juce::String(juce::CharPointer_UTF8("\xe2\x97\x80"))
                            : juce::String(juce::CharPointer_UTF8("\xe2\x96\xb6")),
                       bounds, juce::Justification::centred);
        };
        paintArrow(subPickerLeftArrow_, true);
        paintArrow(subPickerRightArrow_, false);

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(7.0f));
        g.drawText(title, area.removeFromTop(area.getHeight() / 2), juce::Justification::centred);
        g.setFont(fonts::value(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText(indexText, area, juce::Justification::centred);
    }

    void EngineOscillatorPicker::paintSubPicker(juce::Graphics& g)
    {
        if (subPickerBounds_.isEmpty())
            return;

        subPillLayout_.clear();
        subPickerLeftArrow_ = {};
        subPickerRightArrow_ = {};
        subPickerUsesCycler_ = false;

        const auto idx = static_cast<std::size_t>(engineIndex_);
        const auto engine = currentEngineType();
        const auto area = subPickerBounds_;

        switch (engine)
        {
            case algorithm::EngineType::Classic:
            {
                static constexpr const char* kLabels[] = {"SIN", "SAW", "TRI", "SQR", "PLS"};
                paintSubPickerPills(g, area, {kLabels, kLabels + 5}, activeSubPickerIndex_);
                break;
            }
            case algorithm::EngineType::Wavetable:
            {
                static constexpr const char* kLabels[] = {"SPEC", "FADE", "FORM"};
                paintSubPickerLabeled(g, area, "MORPH", {kLabels, kLabels + 3}, activeSubPickerIndex_);
                break;
            }
            case algorithm::EngineType::Granular:
                paintSubPickerSingleButton(g, area, "SAMPLE", "LOAD...");
                break;
            case algorithm::EngineType::FmPm:
            {
                subPickerUsesCycler_ = true;
                const int alg = juce::jlimit(1, 8, activeSubPickerIndex_ + 1);
                paintSubPickerCycler(g, area, "ALGORITHM", juce::String(alg).paddedLeft('0', 2));
                break;
            }
            case algorithm::EngineType::Additive:
            {
                static constexpr const char* kLabels[] = {"OFF", "DRAW"};
                paintSubPickerLabeled(g, area, "DRAW", {kLabels, kLabels + 2}, activeSubPickerIndex_);
                break;
            }
            case algorithm::EngineType::PhaseShape:
            {
                static constexpr const char* kLabels[] = {"SAW", "RES", "PLS"};
                paintSubPickerLabeled(g, area, "PHASE", {kLabels, kLabels + 3}, activeSubPickerIndex_);
                break;
            }
            case algorithm::EngineType::NoiseChaos:
            {
                static constexpr const char* kLabels[] = {"WHITE", "PINK", "BROWN", "BLUE", "VIOLET"};
                paintSubPickerPills(g, area, {kLabels, kLabels + 5}, activeSubPickerIndex_);
                break;
            }
            case algorithm::EngineType::Resonator:
            {
                static constexpr const char* kLabels[] = {"STRING", "TUBE", "PLATE", "BELL", "NOISE", "IMPULSE", "EXT"};
                paintSubPickerPills(g, area, {kLabels, kLabels + 7}, activeSubPickerIndex_);
                break;
            }
            case algorithm::EngineType::External:
            {
                static constexpr const char* kLabels[] = {"AUD1", "AUD2", "S.CH", "RESMP"};
                paintSubPickerPills(g, area, {kLabels, kLabels + 4}, activeSubPickerIndex_);
                break;
            }
            default:
                juce::ignoreUnused(idx);
                break;
        }
    }

    int EngineOscillatorPicker::subPickerPillIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < static_cast<int>(subPillLayout_.size()); ++i)
        {
            if (subPillLayout_[static_cast<std::size_t>(i)].contains(pos))
                return i;
        }
        return -1;
    }

    void EngineOscillatorPicker::activateSubPickerPill(int pillIndex)
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        auto& apvts = processor_.apvts;

        auto setParam = [&](const char* id, float value) {
            if (auto* param = apvts.getParameter(operatorParamId(op, id)))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        };

        activeSubPickerIndex_ = pillIndex;

        switch (currentEngineType())
        {
            case algorithm::EngineType::Classic:
            {
                const int wf = subPickerIndexToWaveform(pillIndex);
                setParam("Waveform", static_cast<float>(wf));
                activeContextCell_ = wf;
                break;
            }
            case algorithm::EngineType::Wavetable:
                setParam("WtMorphMode", static_cast<float>(pillIndex));
                break;
            case algorithm::EngineType::Granular:
                loadGranularSampleFromFile();
                break;
            case algorithm::EngineType::FmPm:
                setParam("FmModulatorWaveform", static_cast<float>(pillIndex));
                activeContextCell_ = juce::jlimit(0, 3, pillIndex);
                break;
            case algorithm::EngineType::Additive:
                setParam("AdditiveOddEven", pillIndex == 1 ? 1.0f : 0.0f);
                if (pillIndex == 1)
                    setParam("AdditivePartialCount", 16.0f);
                activateContextCell(pillIndex);
                break;
            case algorithm::EngineType::PhaseShape:
                setParam("PhaseShape", static_cast<float>(pillIndex) / 2.0f);
                activateContextCell(pillIndex);
                break;
            case algorithm::EngineType::NoiseChaos:
                setParam("NoiseVariant", static_cast<float>(juce::jmin(pillIndex, 4)));
                activeContextCell_ = pillIndex;
                break;
            case algorithm::EngineType::Resonator:
                setParam("ResonatorStructure", static_cast<float>(pillIndex) / 6.0f);
                activateContextCell(juce::jmin(pillIndex, 3));
                break;
            case algorithm::EngineType::External:
                setParam("ExternalInputSource", static_cast<float>(pillIndex));
                activeSubPickerIndex_ = pillIndex;
                break;
            default:
                break;
        }

        refreshPreviews();
        repaint();
    }

    void EngineOscillatorPicker::stepSubPickerCycler(int delta)
    {
        const auto op = static_cast<std::size_t>(engineIndex_);
        auto& apvts = processor_.apvts;

        switch (currentEngineType())
        {
            case algorithm::EngineType::FmPm:
            {
                activeSubPickerIndex_ = juce::jlimit(0, 7, activeSubPickerIndex_ + delta);
                if (auto* param = apvts.getParameter(operatorParamId(op, "FmModulatorWaveform")))
                    param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(activeSubPickerIndex_)));
                activeContextCell_ = juce::jlimit(0, 3, activeSubPickerIndex_);
                break;
            }
            default:
                break;
        }

        refreshPreviews();
        repaint();
    }

    void EngineOscillatorPicker::paintPlayBoardStub(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const auto engine = currentEngineType();
        const auto idx = static_cast<std::size_t>(engineIndex_);

        if (engine == algorithm::EngineType::Wavetable || engine == algorithm::EngineType::Granular)
        {
            if (const auto* table = processor_.getActiveWavetableTable(idx); table != nullptr && table->isValid())
            {
                const float pos = loadParam(processor_.apvts, operatorParamId(idx, "WavetablePos"), 0.0f);
                const auto warpParams = loadWavetableWarpParams(processor_.apvts, idx);
                auto waveArea = bounds.reduced(2.0f, 4.0f);
                wireframe::paintWavetableFrameWaveform(g, waveArea, table, pos, warpParams, engineLive_,
                                                       wireframe::kPreviewPoints);
                if (engine == algorithm::EngineType::Granular)
                {
                    wireframe::GranularOverlayParams grainParams;
                    grainParams.wavetablePos = pos;
                    grainParams.grainSizeMs =
                        loadParam(processor_.apvts, operatorParamId(idx, "GrainSizeMs"), 60.0f);
                    wireframe::paintGranularGrainOverlay(g, waveArea, grainParams);
                }
                return;
            }
        }

        const float stubW = juce::jmin(32.0f, bounds.getWidth());
        const float stubH = 10.0f;
        const float gap = 2.0f;

        auto paintStub = [&](juce::Rectangle<float> stub, bool accent) {
            g.setColour(accent ? palette::kPanelRaised : palette::kPanel);
            g.fillRoundedRectangle(stub, 2.0f);
            g.setColour(palette::kBorder.withAlpha(accent ? 0.85f : 0.65f));
            g.drawRoundedRectangle(stub.reduced(0.5f), 2.0f, 1.0f);
        };

        auto topStub = bounds.removeFromTop(stubH);
        topStub.setWidth(stubW);
        paintStub(topStub, true);

        bounds.removeFromTop(gap);
        auto bottomStub = bounds.removeFromTop(stubH);
        bottomStub.setWidth(stubW);
        paintStub(bottomStub, false);

        auto iconArea = bounds.withSizeKeepingCentre(juce::jmin(18.0f, bounds.getWidth()), juce::jmin(14.0f, bounds.getHeight()));
        if (!iconArea.isEmpty())
        {
            engineicons::drawEngineIcon(g, currentEngineType(), iconArea,
                                        palette::kAccent.withAlpha(engineLive_ ? 0.92f : 0.55f), 1.0f);
        }
    }

    void EngineOscillatorPicker::resized()
    {
        if (playBoardCompactMode_)
            return;

        auto bounds = getLocalBounds();
        const int stripHeight = designModeV2Layout_ ? kDesignModeV2TypeStripHeight : kEngineOscStripHeight;
        auto strip = bounds.removeFromTop(stripHeight);

        if (designModeV2Layout_)
        {
            bounds.removeFromTop(kDesignModeV2CardRowGap);
            subPickerBounds_ = bounds.removeFromTop(kDesignModeV2SubPickerHeight);
            bounds.removeFromTop(layout::kDesignModeV2CardRowGapAfterSubPicker);
            contextPreviewBounds_ = bounds;

            contextLeftArrow_ = {};
            contextRightArrow_ = {};
            contextUsesPositionArrows_ = usesWavetablePositionControls();
            if (contextUsesPositionArrows_)
            {
                auto contextArea = contextPreviewBounds_.reduced(1);
                contextLeftArrow_ = contextArea.removeFromLeft(kDesignModeV2SubPickerArrowWidth)
                                        .withSizeKeepingCentre(kDesignModeV2SubPickerArrowWidth, 28);
                contextRightArrow_ = contextArea.removeFromRight(kDesignModeV2SubPickerArrowWidth)
                                         .withSizeKeepingCentre(kDesignModeV2SubPickerArrowWidth, 28);
                contextThumb_.setBounds(contextArea);
            }
            else
            {
                contextThumb_.setBounds(contextPreviewBounds_.reduced(1));
            }
        }
        else
        {
            bounds.removeFromTop(kEngineOscStripGridGap);

            const int gridTop = strip.getBottom() + kEngineOscStripGridGap;
            const int row1Y = gridTop;
            const int row2Y = row1Y + kEngineOscCellHeight + kEngineOscCellGap;
            cellLayout_[0] = {bounds.getX(), row1Y, kEngineOscCellWidth, kEngineOscCellHeight};
            cellLayout_[1] = {bounds.getX() + kEngineOscCellWidth + kEngineOscCellGap, row1Y, kEngineOscCellWidth,
                              kEngineOscCellHeight};
            cellLayout_[2] = {bounds.getX(), row2Y, kEngineOscCellWidth, kEngineOscCellHeight};
            cellLayout_[3] = {bounds.getX() + kEngineOscCellWidth + kEngineOscCellGap, row2Y, kEngineOscCellWidth,
                              kEngineOscCellHeight};
            juce::ignoreUnused(kEngineOscContextGridHeight);
        }

        typePillLayout_.clear();
        const int pills = numTypePills();
        const int pillW = juce::jmax(1, strip.getWidth() / pills);
        for (int i = 0; i < pills; ++i)
        {
            const int pillX = strip.getX() + i * pillW;
            const int pillWidth = (i == pills - 1) ? strip.getRight() - pillX : pillW;
            typePillLayout_.push_back({pillX, strip.getY(), pillWidth, stripHeight});
        }
    }

} // namespace pw8::plugin::ui
