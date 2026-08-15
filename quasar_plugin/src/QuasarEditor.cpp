#include "QuasarEditor.h"

#include <array>

#include "ui/components/GlowKnob.h"
#include "ui/components/MetadataFacetRow.h"
#include "ui/theme/ObsidianFonts.h"
#include "ui/theme/ObsidianPalette.h"

namespace pw8::quasar
{
    namespace
    {
        [[nodiscard]] int readIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        {
            if (auto* param = apvts.getParameter(id))
                return static_cast<int>(std::lround(param->convertFrom0to1(param->getValue())));
            return 0;
        }

        [[nodiscard]] juce::String outputModeLabel(float value)
        {
            switch (static_cast<int>(std::lround(value)))
            {
                case 1: return "SPEAKER";
                case 2: return "AUTO";
                default: return "PHONE";
            }
        }
    } // namespace

    QuasarEditor::QuasarEditor(QuasarProcessor& processor)
        : juce::AudioProcessorEditor(processor), processor_(processor)
    {
        setLookAndFeel(&laf_);
        setSize(920, 640);

        titleLabel_.setText("QUASAR", juce::dontSendNotification);
        titleLabel_.setFont(ui::fonts::title(22.0f));
        titleLabel_.setColour(juce::Label::textColourId, ui::palette::kAccent);
        titleLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel_);

        helpLabel_.setText(
            "Headphone-first binaural spatial mixer. CNTR keeps bass mono-stable; QSR1/QSR2 orbit outside the head. "
            "AU sidechain sums into the processor input (QSR2 aux foundation).",
            juce::dontSendNotification);
        helpLabel_.setJustificationType(juce::Justification::centredLeft);
        helpLabel_.setFont(ui::fonts::value(10.0f));
        helpLabel_.setColour(juce::Label::textColourId, ui::palette::kTextDim);
        addAndMakeVisible(helpLabel_);

        scopeLabel_.setText("Spherical scope — Phase 4", juce::dontSendNotification);
        scopeLabel_.setJustificationType(juce::Justification::centred);
        scopeLabel_.setFont(ui::fonts::label(11.0f));
        scopeLabel_.setColour(juce::Label::textColourId, ui::palette::kTextSecondary);
        scopeLabel_.setColour(juce::Label::backgroundColourId, ui::palette::kPanelRaised);
        addAndMakeVisible(scopeLabel_);

        sidechainLabel_.setText("Sidechain (AU): summed with main input for MVP", juce::dontSendNotification);
        sidechainLabel_.setJustificationType(juce::Justification::centredLeft);
        sidechainLabel_.setFont(ui::fonts::label(10.0f));
        sidechainLabel_.setColour(juce::Label::textColourId, ui::palette::kAccentWarm);
        addAndMakeVisible(sidechainLabel_);

        delaySyncRow_ = std::make_unique<ui::MetadataFacetRow>("DLY SYNC");
        delaySyncRow_->setValues(juce::StringArray{"FREE", "TEMPO"});
        delaySyncRow_->onChange = [this]() {
            auto& apvts = processor_.getApvts();
            const bool sync = delaySyncRow_->getSelectedValue() == "TEMPO";
            if (auto* param = apvts.getParameter("quasarDelaySync"))
                param->setValueNotifyingHost(sync ? 1.0f : 0.0f);
            if (delayDivisionRow_ != nullptr)
                delayDivisionRow_->setVisible(sync);
            resized();
        };
        addAndMakeVisible(*delaySyncRow_);

        delayDivisionRow_ = std::make_unique<ui::MetadataFacetRow>("DLY DIV");
        delayDivisionRow_->setValues(
            juce::StringArray{"1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "8/1"});
        delayDivisionRow_->onChange = [this]() {
            auto& apvts = processor_.getApvts();
            static constexpr const char* kLabels[] = {"1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "8/1"};
            const auto& selected = delayDivisionRow_->getSelectedValue();
            int index = 2;
            for (int i = 0; i < 9; ++i)
            {
                if (selected.equalsIgnoreCase(kLabels[i]))
                {
                    index = i;
                    break;
                }
            }
            if (auto* param = apvts.getParameter("quasarDelaySyncDivision"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(index)));
        };
        addAndMakeVisible(*delayDivisionRow_);

        rebuildKnobs();
        startTimerHz(4);
    }

    QuasarEditor::~QuasarEditor()
    {
        setLookAndFeel(nullptr);
    }

    void QuasarEditor::rebuildKnobs()
    {
        knobs_.clear();
        auto& apvts = processor_.getApvts();

        struct KnobSpec
        {
            const char* id;
            const char* label;
            juce::Colour accent;
            std::function<juce::String(float)> formatter;
        };

        const std::array<KnobSpec, 22> specs = {{
            {"mix", "MIX", ui::palette::kAccent, nullptr},
            {"cntrLevel", "CNTR", ui::palette::kAccentWarm, nullptr},
            {"qsr1Level", "QSR1", ui::palette::kMurmurViolet, nullptr},
            {"qsr2Level", "QSR2", ui::palette::kMurmurVioletDeep, nullptr},
            {"qsr1Distance", "Q1 DIST", juce::Colours::transparentBlack, nullptr},
            {"qsr1Angle", "Q1 ANGLE", juce::Colours::transparentBlack, nullptr},
            {"qsr1Height", "Q1 HEIGHT", juce::Colours::transparentBlack, nullptr},
            {"qsr2Distance", "Q2 DIST", juce::Colours::transparentBlack, nullptr},
            {"qsr2Angle", "Q2 ANGLE", juce::Colours::transparentBlack, nullptr},
            {"qsr2Height", "Q2 HEIGHT", juce::Colours::transparentBlack, nullptr},
            {"qsr1RoomAmount", "Q1 ROOM", juce::Colours::transparentBlack, nullptr},
            {"qsr1RoomSize", "Q1 SIZE", juce::Colours::transparentBlack, nullptr},
            {"qsr1RoomDamping", "Q1 DAMP", juce::Colours::transparentBlack, nullptr},
            {"qsr2RoomAmount", "Q2 ROOM", juce::Colours::transparentBlack, nullptr},
            {"qsr2RoomSize", "Q2 SIZE", juce::Colours::transparentBlack, nullptr},
            {"qsr2RoomDamping", "Q2 DAMP", juce::Colours::transparentBlack, nullptr},
            {"quasarDelayTimeMs", "DLY TIME", juce::Colours::transparentBlack,
             [](float v) { return juce::String(v, 0) + " ms"; }},
            {"quasarDelayFeedback", "DLY FDBK", juce::Colours::transparentBlack, nullptr},
            {"quasarDelayVolume", "DLY VOL", juce::Colours::transparentBlack, nullptr},
            {"quasarOutputMode", "OUTPUT", ui::palette::kAccent, outputModeLabel},
            {"quasarCrossfeed", "X-FEED", juce::Colours::transparentBlack, nullptr},
            {"inputSplitHpfHz", "QSR HPF", juce::Colours::transparentBlack,
             [](float v) { return juce::String(v, 0) + " Hz"; }},
        }};

        for (const auto& spec : specs)
        {
            auto knob = std::make_unique<ui::GlowKnob>(apvts, spec.id, spec.label, spec.formatter, spec.accent);
            addAndMakeVisible(*knob);
            knobs_.push_back(std::move(knob));
        }

        juce::ignoreUnused(apvts);
    }

    void QuasarEditor::timerCallback()
    {
        auto& apvts = processor_.getApvts();
        const bool syncOn = readIntParam(apvts, "quasarDelaySync") != 0;
        const int div = readIntParam(apvts, "quasarDelaySyncDivision");
        if (delaySyncRow_ != nullptr)
            delaySyncRow_->setSelectedValue(syncOn ? "TEMPO" : "FREE");
        if (delayDivisionRow_ != nullptr)
        {
            static constexpr const char* kLabels[] = {"1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1", "8/1"};
            delayDivisionRow_->setSelectedValue(kLabels[juce::jlimit(0, 8, div)]);
            delayDivisionRow_->setVisible(syncOn);
        }
    }

    void QuasarEditor::paint(juce::Graphics& g)
    {
        juce::ColourGradient bg(ui::palette::kBackgroundTop, 0.0f, 0.0f, ui::palette::kBackgroundBottom, 0.0f,
                                static_cast<float>(getHeight()), false);
        g.setGradientFill(bg);
        g.fillAll();
    }

    void QuasarEditor::layoutKnobGrid(juce::Rectangle<int> area, const std::vector<juce::Component*>& knobPtrs, int cols)
    {
        if (knobPtrs.empty())
            return;
        const int rows = static_cast<int>((knobPtrs.size() + static_cast<std::size_t>(cols) - 1) /
                                          static_cast<std::size_t>(cols));
        const int rowH = juce::jmin(92, area.getHeight() / juce::jmax(1, rows));
        std::size_t idx = 0;
        for (int r = 0; r < rows && area.getHeight() > 0; ++r)
        {
            auto row = area.removeFromTop(rowH);
            const int kw = row.getWidth() / cols;
            for (int c = 0; c < cols && idx < knobPtrs.size(); ++c, ++idx)
                knobPtrs[idx]->setBounds(row.removeFromLeft(kw).reduced(3));
        }
    }

    void QuasarEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(12);
        titleLabel_.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(4);
        helpLabel_.setBounds(bounds.removeFromTop(36));
        bounds.removeFromTop(6);
        scopeLabel_.setBounds(bounds.removeFromTop(100));
        bounds.removeFromTop(6);
        sidechainLabel_.setBounds(bounds.removeFromTop(16));
        bounds.removeFromTop(6);

        if (delaySyncRow_ != nullptr)
            delaySyncRow_->setBounds(bounds.removeFromTop(34));
        bounds.removeFromTop(4);
        if (delayDivisionRow_ != nullptr && delayDivisionRow_->isVisible())
            delayDivisionRow_->setBounds(bounds.removeFromTop(34));
        bounds.removeFromTop(8);

        std::vector<juce::Component*> knobPtrs;
        knobPtrs.reserve(knobs_.size());
        for (auto& knob : knobs_)
            knobPtrs.push_back(knob.get());
        layoutKnobGrid(bounds, knobPtrs, 6);
    }

} // namespace pw8::quasar
