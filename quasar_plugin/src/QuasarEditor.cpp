#include "QuasarEditor.h"

#include <array>

#include "ui/components/GlowKnob.h"
#include "ui/components/MetadataFacetRow.h"
#include "ui/components/QuasarSpatialWireframeView.h"
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

        titleLabel_.setText("QUASAR", juce::dontSendNotification);
        titleLabel_.setFont(ui::fonts::title(22.0f));
        titleLabel_.setColour(juce::Label::textColourId, ui::palette::kAccent);
        titleLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(titleLabel_);

        helpLabel_.setText(
            "Stereo split: L → QSR1 (cyan), R → QSR2 (violet). Drag scope markers or use PLAY knobs. "
            "ORBIT/SPREAD macros fan both feeds; AU sidechain can feed QSR2 only.",
            juce::dontSendNotification);
        helpLabel_.setJustificationType(juce::Justification::centredLeft);
        helpLabel_.setFont(ui::fonts::value(10.0f));
        helpLabel_.setColour(juce::Label::textColourId, ui::palette::kTextDim);
        addAndMakeVisible(helpLabel_);

        playLegendLabel_.setText("PLAY · 2 macro KOINS + 6 spatial controls", juce::dontSendNotification);
        playLegendLabel_.setJustificationType(juce::Justification::centredLeft);
        playLegendLabel_.setFont(ui::fonts::label(10.0f));
        playLegendLabel_.setColour(juce::Label::textColourId, ui::palette::kAccentWarm);
        addAndMakeVisible(playLegendLabel_);

        sidechainLabel_.setText("AU Sidechain: QSR2-only aux (default) or legacy sum-with-main", juce::dontSendNotification);
        sidechainLabel_.setJustificationType(juce::Justification::centredLeft);
        sidechainLabel_.setFont(ui::fonts::label(10.0f));
        sidechainLabel_.setColour(juce::Label::textColourId, ui::palette::kAccentWarm);
        addAndMakeVisible(sidechainLabel_);

        spatialScope_ = std::make_unique<ui::QuasarSpatialWireframeView>(processor_.getApvts());
        addAndMakeVisible(*spatialScope_);

        sidechainModeRow_ = std::make_unique<ui::MetadataFacetRow>("SC AUX");
        sidechainModeRow_->setValues(juce::StringArray{"QSR2", "SUM"});
        sidechainModeRow_->onChange = [this]() {
            auto& apvts = processor_.getApvts();
            const bool qsr2Only = sidechainModeRow_->getSelectedValue() == "QSR2";
            if (auto* param = apvts.getParameter("sidechainToQsr2"))
                param->setValueNotifyingHost(qsr2Only ? 1.0f : 0.0f);
        };
        addAndMakeVisible(*sidechainModeRow_);

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

        deepToggle_.setClickingTogglesState(true);
        deepToggle_.setToggleState(false, juce::dontSendNotification);
        deepToggle_.onClick = [this]() { toggleDeepPanel(); };
        addAndMakeVisible(deepToggle_);

        rebuildMacroKnobs();
        rebuildPlayKnobs();
        rebuildDeepKnobs();

        setResizable(true, true);
        setResizeLimits(720, 520, 1400, 900);
        setSize(920, 720);
        startTimerHz(15);
    }

    QuasarEditor::~QuasarEditor()
    {
        stopTimer();
        setLookAndFeel(nullptr);
    }

    void QuasarEditor::rebuildMacroKnobs()
    {
        macroKnobs_.clear();
        auto& apvts = processor_.getApvts();

        struct MacroSpec
        {
            const char* id;
            const char* label;
            juce::Colour accent;
        };

        const std::array<MacroSpec, 2> specs = {{
            {"orbitMacro", "ORBIT", ui::palette::kAccentWarm},
            {"spreadMacro", "SPREAD", ui::palette::kMurmurVioletDeep},
        }};

        for (const auto& spec : specs)
        {
            auto knob = std::make_unique<ui::GlowKnob>(apvts, spec.id, spec.label, nullptr, spec.accent);
            addAndMakeVisible(*knob);
            macroKnobs_.push_back(std::move(knob));
        }
    }

    void QuasarEditor::rebuildPlayKnobs()
    {
        playKnobs_.clear();
        auto& apvts = processor_.getApvts();

        struct PlayKnobSpec
        {
            const char* id;
            const char* label;
            juce::Colour accent;
            std::function<juce::String(float)> formatter;
        };

        const std::array<PlayKnobSpec, 6> specs = {{
            {"qsr1Angle", "L ORBIT", ui::palette::kAccent,
             [](float v) { return juce::String(v, 0) + juce::String::fromUTF8("°"); }},
            {"qsr1Distance", "L DEPTH", ui::palette::kAccent, nullptr},
            {"qsr1Height", "L LIFT", ui::palette::kAccent, nullptr},
            {"qsr2Angle", "R ORBIT", ui::palette::kMurmurViolet,
             [](float v) { return juce::String(v, 0) + juce::String::fromUTF8("°"); }},
            {"qsr2Distance", "R DEPTH", ui::palette::kMurmurVioletDeep, nullptr},
            {"qsr2Height", "R LIFT", ui::palette::kMurmurViolet, nullptr},
        }};

        for (const auto& spec : specs)
        {
            auto knob = std::make_unique<ui::GlowKnob>(apvts, spec.id, spec.label, spec.formatter, spec.accent);
            addAndMakeVisible(*knob);
            playKnobs_.push_back(std::move(knob));
        }
    }

    void QuasarEditor::rebuildDeepKnobs()
    {
        deepKnobs_.clear();
        auto& apvts = processor_.getApvts();

        struct KnobSpec
        {
            const char* id;
            const char* label;
            std::function<juce::String(float)> formatter;
        };

        const std::array<KnobSpec, 17> specs = {{
            {"mix", "MIX", nullptr},
            {"cntrLevel", "CNTR", nullptr},
            {"qsr1Level", "QSR1 LVL", nullptr},
            {"qsr2Level", "QSR2 LVL", nullptr},
            {"qsr1RoomAmount", "L ROOM", nullptr},
            {"qsr2RoomAmount", "R ROOM", nullptr},
            {"qsr1RoomSize", "L SIZE", nullptr},
            {"qsr2RoomSize", "R SIZE", nullptr},
            {"quasarDelayTimeMs", "DLY TIME", [](float v) { return juce::String(v, 0) + " ms"; }},
            {"quasarDelayFeedback", "DLY FDBK", nullptr},
            {"quasarDelayVolume", "DLY VOL", nullptr},
            {"quasarOutputMode", "OUTPUT", outputModeLabel},
            {"quasarCrossfeed", "X-FEED", nullptr},
            {"inputSplitHpfHz", "QSR HPF", [](float v) { return juce::String(v, 0) + " Hz"; }},
            {"cntrHpfHz", "CNTR HPF", [](float v) { return juce::String(v, 0) + " Hz"; }},
            {"qsr1RoomDamping", "L DAMP", nullptr},
            {"qsr2RoomDamping", "R DAMP", nullptr},
        }};

        for (const auto& spec : specs)
        {
            auto knob = std::make_unique<ui::GlowKnob>(apvts, spec.id, spec.label, spec.formatter);
            knob->setVisible(deepPanelOpen_);
            addAndMakeVisible(*knob);
            deepKnobs_.push_back(std::move(knob));
        }
    }

    void QuasarEditor::toggleDeepPanel()
    {
        deepPanelOpen_ = deepToggle_.getToggleState();
        deepToggle_.setButtonText(deepPanelOpen_ ? "DEEP ▴" : "DEEP ▾");
        for (auto& knob : deepKnobs_)
            knob->setVisible(deepPanelOpen_);
        resized();
    }

    void QuasarEditor::timerCallback()
    {
        if (spatialScope_ != nullptr)
            spatialScope_->repaint();

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
        if (sidechainModeRow_ != nullptr)
        {
            const bool qsr2Only = readIntParam(apvts, "sidechainToQsr2") != 0;
            sidechainModeRow_->setSelectedValue(qsr2Only ? "QSR2" : "SUM");
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
        if (spatialScope_ == nullptr)
            return;

        auto bounds = getLocalBounds().reduced(12);
        titleLabel_.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(4);
        helpLabel_.setBounds(bounds.removeFromTop(40));
        bounds.removeFromTop(4);
        playLegendLabel_.setBounds(bounds.removeFromTop(16));
        bounds.removeFromTop(6);

        auto topRow = bounds.removeFromTop(220);
        spatialScope_->setBounds(topRow.removeFromLeft(topRow.getWidth() * 2 / 3).reduced(0, 2));

        auto playColumn = topRow.reduced(4, 2);
        auto macroRow = playColumn.removeFromTop(88);
        std::vector<juce::Component*> macroPtrs;
        macroPtrs.reserve(macroKnobs_.size());
        for (auto& knob : macroKnobs_)
            macroPtrs.push_back(knob.get());
        layoutKnobGrid(macroRow, macroPtrs, 2);

        playColumn.removeFromTop(4);
        std::vector<juce::Component*> playPtrs;
        playPtrs.reserve(playKnobs_.size());
        for (auto& knob : playKnobs_)
            playPtrs.push_back(knob.get());
        layoutKnobGrid(playColumn, playPtrs, 2);

        bounds.removeFromTop(6);
        sidechainLabel_.setBounds(bounds.removeFromTop(16));
        bounds.removeFromTop(4);

        auto utilityRow = bounds.removeFromTop(34);
        deepToggle_.setBounds(utilityRow.removeFromLeft(72));
        utilityRow.removeFromLeft(8);
        if (sidechainModeRow_ != nullptr)
            sidechainModeRow_->setBounds(utilityRow.removeFromLeft(150));
        utilityRow.removeFromLeft(8);
        if (delaySyncRow_ != nullptr)
            delaySyncRow_->setBounds(utilityRow.removeFromLeft(160));
        utilityRow.removeFromLeft(8);
        if (delayDivisionRow_ != nullptr && delayDivisionRow_->isVisible())
            delayDivisionRow_->setBounds(utilityRow.removeFromLeft(260));

        if (deepPanelOpen_)
        {
            bounds.removeFromTop(8);
            std::vector<juce::Component*> deepPtrs;
            deepPtrs.reserve(deepKnobs_.size());
            for (auto& knob : deepKnobs_)
                deepPtrs.push_back(knob.get());
            layoutKnobGrid(bounds, deepPtrs, 6);
        }
    }

} // namespace pw8::quasar
