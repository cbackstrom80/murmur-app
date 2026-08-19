#include "MasterQuasarPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/content/ContentPaths.hpp"
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

        void setParamValue(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        }

        void setBoolParamValue(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, bool value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(value ? 1.0f : 0.0f);
        }

        [[nodiscard]] float pickQuasarParam(const juce::var& params, std::initializer_list<const char*> keys,
                                          float fallback)
        {
            if (!params.isObject())
                return fallback;
            for (const char* key : keys)
            {
                if (params.hasProperty(key))
                    return static_cast<float>(params.getProperty(key, fallback));
            }
            return fallback;
        }
    } // namespace

    class MasterQuasarPanel::ScrollContent : public juce::Component
    {
    public:
        QuasarSegmentedMeter* meter = nullptr;
        QuasarBinauralFieldView* field = nullptr;
        QuasarPrimaryKnobRow* primaryRow = nullptr;
        GlowKnob* orbitMacro = nullptr;
        GlowKnob* spreadMacro = nullptr;
        QuasarEngineCard* engine = nullptr;
        QuasarSpatialCard* spatial = nullptr;
        QuasarTelemetryBar* telemetry = nullptr;

        void resized() override
        {
            auto bounds = getLocalBounds();
            const int gap = layout::kDesignFxPageSectionGap;

            auto heroRow = bounds.removeFromTop(layout::kQuasarBinauralFieldHeight);
            if (meter != nullptr)
                meter->setBounds(heroRow.removeFromLeft(96).withHeight(160));
            heroRow.removeFromLeft(gap);
            if (field != nullptr)
                field->setBounds(heroRow);

            bounds.removeFromTop(gap);
            if (primaryRow != nullptr)
                primaryRow->setBounds(bounds.removeFromTop(layout::kQuasarPrimaryKnobRowHeight));
            bounds.removeFromTop(gap);

            auto cards = bounds.removeFromTop(layout::kQuasarBottomCardHeight);
            const int cardGap = gap;
            const int cardW = (cards.getWidth() - cardGap) / 2;
            if (engine != nullptr)
                engine->setBounds(cards.removeFromLeft(cardW));
            cards.removeFromLeft(cardGap);
            if (spatial != nullptr)
                spatial->setBounds(cards);

            bounds.removeFromTop(gap);
            if (telemetry != nullptr)
                telemetry->setBounds(bounds.removeFromTop(layout::kQuasarTelemetryBarHeight));

            setSize(getWidth(), getPreferredHeight());
        }

        int getPreferredHeight() const
        {
            return layout::kQuasarBinauralFieldHeight + layout::kDesignFxPageSectionGap
                   + layout::kQuasarPrimaryKnobRowHeight + layout::kDesignFxPageSectionGap
                   + layout::kQuasarBottomCardHeight + layout::kDesignFxPageSectionGap
                   + layout::kQuasarTelemetryBarHeight + 24;
        }
    };

    MasterQuasarPanel::MasterQuasarPanel(PatchworkEightProcessor& processor)
        : processor_(processor),
          apvts_(processor.apvts),
          chainHeader_(processor, apvts_),
          segmentedMeter_(processor),
          primaryKnobRow_(processor, apvts_),
          engineCard_(processor, apvts_),
          spatialCard_(processor, apvts_),
          telemetryBar_(processor)
    {
        addAndMakeVisible(backButton_);
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };

        specBadge_.setText("102:4", juce::dontSendNotification);
        specBadge_.setFont(fonts::label(9.0f));
        specBadge_.setColour(juce::Label::textColourId, juce::Colour(0xff7c4dff));
        specBadge_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(specBadge_);

        titleLabel_.setText("QUASAR BINAURAL", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("Master-bus spatializer — azimuth ring + QSR drag", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(10.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        importQuasarButton_.setButtonText("IMPORT .QUASAR");
        importQuasarButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        importQuasarButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7c4dff));
        importQuasarButton_.onClick = [this] { importCompanionQuasar(); };
        addAndMakeVisible(importQuasarButton_);

        openFxChainButton_.setButtonText("OPEN FX CHAIN");
        openFxChainButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        openFxChainButton_.setColour(juce::TextButton::textColourOffId, palette::kTextPrimary);
        openFxChainButton_.onClick = [this] {
            if (onOpenFxChain)
                onOpenFxChain();
        };
        addAndMakeVisible(openFxChainButton_);

        chainHeader_.onSlotSelected = [this](std::size_t slot) { bindSlot(slot); };
        addAndMakeVisible(chainHeader_);

        fieldView_ = std::make_unique<QuasarBinauralFieldView>(apvts_);
        fieldView_->attachVisualizerBus(processor_.getVisualizerBus());

        orbitMacroKnob_ = std::make_unique<GlowKnob>(apvts_, kMacroParameterIds[3], "ORBIT");
        orbitMacroKnob_->applyFigmaContext(figma::KnobContext::DesignVocoder);
        spreadMacroKnob_ = std::make_unique<GlowKnob>(apvts_, kMacroParameterIds[6], "SPREAD");
        spreadMacroKnob_->applyFigmaContext(figma::KnobContext::DesignVocoder);

        auto* content = new ScrollContent();
        scrollContent_ = content;
        content->meter = &segmentedMeter_;
        content->field = fieldView_.get();
        content->primaryRow = &primaryKnobRow_;
        content->orbitMacro = orbitMacroKnob_.get();
        content->spreadMacro = spreadMacroKnob_.get();
        content->engine = &engineCard_;
        content->spatial = &spatialCard_;
        content->telemetry = &telemetryBar_;

        content->addAndMakeVisible(segmentedMeter_);
        content->addAndMakeVisible(*fieldView_);
        content->addAndMakeVisible(primaryKnobRow_);
        content->addAndMakeVisible(*orbitMacroKnob_);
        content->addAndMakeVisible(*spreadMacroKnob_);
        content->addAndMakeVisible(engineCard_);
        content->addAndMakeVisible(spatialCard_);
        content->addAndMakeVisible(telemetryBar_);

        viewport_.setViewedComponent(content, true);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);

    }

    MasterQuasarPanel::~MasterQuasarPanel() { stopTimer(); }

    void MasterQuasarPanel::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← FX CARDS" : "← PLAY BOARD");
        openFxChainButton_.setButtonText(embedded ? "OPEN FX CHAIN" : "OPEN FULL FX CHAIN");
        specBadge_.setVisible(!embedded);
        resized();
    }

    juce::String MasterQuasarPanel::slotParamPrefix(std::size_t slotIndex) const
    {
        return masterFxParamId(slotIndex >= 3 ? slotIndex - 3 : 0, "");
    }

    juce::File MasterQuasarPanel::resolveCompanionQuasarFile() const
    {
        juce::String stem;
        if (const auto presetPath = processor_.getCurrentPresetPath(); presetPath.isNotEmpty())
            stem = juce::File(presetPath).getFileNameWithoutExtension();
        else
        {
            const auto& patch = processor_.getCurrentPatch();
            stem = juce::String(patch.metadata.name.c_str()).trim().toLowerCase().replaceCharacter(' ', '-');
        }

        if (stem.isEmpty())
            return {};

        for (const auto& rootStr : pw8::content::presetSearchRoots())
        {
            const juce::File candidate =
                juce::File(rootStr).getParentDirectory()
                    .getChildFile("presets/quasar/interstellar")
                    .getChildFile(stem + ".quasar");
            if (candidate.existsAsFile())
                return candidate;
        }

        const juce::File devCandidate =
            juce::File::getCurrentWorkingDirectory()
                .getChildFile("content/presets/quasar/interstellar")
                .getChildFile(stem + ".quasar");
        if (devCandidate.existsAsFile())
            return devCandidate;

        return {};
    }

    void MasterQuasarPanel::importCompanionQuasar()
    {
        const juce::File quasarFile = resolveCompanionQuasarFile();
        if (!quasarFile.existsAsFile())
            return;

        const auto parsed = juce::JSON::parse(quasarFile.loadFileAsString());
        if (parsed.isVoid())
            return;

        const auto params = parsed.hasProperty("params") ? parsed.getProperty("params", {}) : parsed;
        if (!params.isObject())
            return;

        const auto prefix = slotParamPrefix(slotIndex_);
        setEffectType(apvts_, prefix, 13);

        setParamValue(apvts_, prefix + "Mix", pickQuasarParam(params, {"mix"}, 0.72f));
        setParamValue(apvts_, prefix + "Qsr1Level", pickQuasarParam(params, {"qsr1Level"}, 0.65f));
        setParamValue(apvts_, prefix + "Qsr2Level", pickQuasarParam(params, {"qsr2Level"}, 0.55f));
        setParamValue(apvts_, prefix + "CntrLevel", pickQuasarParam(params, {"cntrLevel"}, 0.85f));
        setParamValue(apvts_, prefix + "InputSplitHpfHz", pickQuasarParam(params, {"inputSplitHpfHz"}, 120.0f));
        setParamValue(apvts_, prefix + "CntrHpfHz", pickQuasarParam(params, {"cntrHpfHz"}, 80.0f));
        setParamValue(apvts_, prefix + "Qsr1Height", pickQuasarParam(params, {"qsr1Height"}, 0.0f));
        setParamValue(apvts_, prefix + "Qsr1AngleDeg",
                      pickQuasarParam(params, {"qsr1AngleDeg", "qsr1Angle"}, 30.0f));
        setParamValue(apvts_, prefix + "Qsr1Distance", pickQuasarParam(params, {"qsr1Distance"}, 0.35f));
        setParamValue(apvts_, prefix + "Qsr2Height", pickQuasarParam(params, {"qsr2Height"}, 0.0f));
        setParamValue(apvts_, prefix + "Qsr2AngleDeg",
                      pickQuasarParam(params, {"qsr2AngleDeg", "qsr2Angle"}, 330.0f));
        setParamValue(apvts_, prefix + "Qsr2Distance", pickQuasarParam(params, {"qsr2Distance"}, 0.4f));
        setParamValue(apvts_, prefix + "Qsr1RoomAmount", pickQuasarParam(params, {"qsr1RoomAmount"}, 0.45f));
        setParamValue(apvts_, prefix + "Qsr1RoomSize", pickQuasarParam(params, {"qsr1RoomSize"}, 1.0f));
        setParamValue(apvts_, prefix + "Qsr1RoomDamping", pickQuasarParam(params, {"qsr1RoomDamping"}, 0.55f));
        setParamValue(apvts_, prefix + "Qsr2RoomAmount", pickQuasarParam(params, {"qsr2RoomAmount"}, 0.40f));
        setParamValue(apvts_, prefix + "Qsr2RoomSize", pickQuasarParam(params, {"qsr2RoomSize"}, 1.1f));
        setParamValue(apvts_, prefix + "Qsr2RoomDamping", pickQuasarParam(params, {"qsr2RoomDamping"}, 0.50f));
        setParamValue(apvts_, prefix + "QuasarDelayTimeMs", pickQuasarParam(params, {"quasarDelayTimeMs"}, 450.0f));
        setParamValue(apvts_, prefix + "QuasarDelayFeedback", pickQuasarParam(params, {"quasarDelayFeedback"}, 0.35f));
        setParamValue(apvts_, prefix + "QuasarDelayVolume", pickQuasarParam(params, {"quasarDelayVolume"}, 0.25f));
        setParamValue(apvts_, prefix + "QuasarOutputMode",
                      pickQuasarParam(params, {"quasarOutputMode"}, 0.0f));
        setParamValue(apvts_, prefix + "QuasarCrossfeed", pickQuasarParam(params, {"quasarCrossfeed"}, 0.0f));
        setBoolParamValue(apvts_, prefix + "QuasarDelaySync", pickQuasarParam(params, {"quasarDelaySync"}, 0.0f) >= 0.5f);
        setParamValue(apvts_, prefix + "QuasarDelaySyncDivision",
                      pickQuasarParam(params, {"quasarDelaySyncDivisionIndex", "quasarDelaySyncDivision"}, 2.0f));

        chainHeader_.refresh();
        engineCard_.refresh();
        spatialCard_.refresh();
    }

    void MasterQuasarPanel::bindSlot(std::size_t slotIndex)
    {
        slotIndex_ = juce::jlimit<std::size_t>(3, 6, slotIndex);

        const auto prefix = slotParamPrefix(slotIndex_);
        if (readEffectType(apvts_, prefix) != 13)
            setEffectType(apvts_, prefix, 13);

        if (fieldView_ != nullptr)
        {
            fieldView_->bindParameters(prefix + "Qsr1AngleDeg", prefix + "Qsr1Distance", prefix + "Qsr1Height",
                                       prefix + "Qsr2AngleDeg", prefix + "Qsr2Distance", prefix + "Qsr2Height");
        }

        chainHeader_.bindSlot(slotIndex_);
        primaryKnobRow_.rebuild(slotIndex_);
        engineCard_.bindSlot(slotIndex_);
        spatialCard_.bindSlot(slotIndex_);

        if (scrollContent_ != nullptr)
        {
            scrollContent_->setSize(viewport_.getWidth(), scrollContent_->getPreferredHeight());
            scrollContent_->resized();
        }
        resized();
    }

    void MasterQuasarPanel::timerCallback()
    {
        if (!isVisible())
            return;

        segmentedMeter_.tick();
        if (fieldView_ != nullptr)
            fieldView_->pushTrailSample();
        engineCard_.tick();
        telemetryBar_.tick();
        chainHeader_.refresh();
    }

    void MasterQuasarPanel::showForFxSlot(std::size_t slotIndex)
    {
        bindSlot(slotIndex);
        if (!isTimerRunning())
            startTimerHz(30);
        setVisible(true);
    }

    void MasterQuasarPanel::dismiss()
    {
        stopTimer();
        setVisible(false);
    }

    void MasterQuasarPanel::paint(juce::Graphics& g) { g.fillAll(palette::kBackgroundBottom); }

    void MasterQuasarPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(16);
        auto header = bounds.removeFromTop(28);
        backButton_.setBounds(header.removeFromLeft(120));
        specBadge_.setBounds(header.removeFromRight(48));
        titleLabel_.setBounds(header.removeFromLeft(160));
        subtitleLabel_.setBounds(header);

        bounds.removeFromTop(8);
        auto toolbar = bounds.removeFromTop(28);
        importQuasarButton_.setBounds(toolbar.removeFromLeft(140));
        toolbar.removeFromLeft(8);
        openFxChainButton_.setBounds(toolbar.removeFromRight(180));

        bounds.removeFromTop(8);
        chainHeader_.setBounds(bounds.removeFromTop(36));

        bounds.removeFromTop(8);
        viewport_.setBounds(bounds);

        if (scrollContent_ != nullptr)
        {
            scrollContent_->setSize(viewport_.getWidth(), scrollContent_->getPreferredHeight());
            scrollContent_->resized();
        }
    }

    bool MasterQuasarPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
