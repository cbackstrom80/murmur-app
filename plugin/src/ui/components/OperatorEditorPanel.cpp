#include "OperatorEditorPanel.h"

#include "AlgorithmGraphView.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNumEngines = 8;
        constexpr int kPillRowHeight = 30;
        constexpr int kNoteHeight = 16;

        juce::String waveformToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "SINE";
                case 1: return "TRIANGLE";
                case 2: return "SAW";
                case 3: return "SQUARE";
                default: return juce::String(v);
            }
        }

        algorithm::EngineType engineForIndex(int index) noexcept
        {
            return static_cast<algorithm::EngineType>(juce::jlimit(0, kNumEngines - 1, index));
        }
    } // namespace

    OperatorEditorPanel::OperatorEditorPanel(PatchworkEightProcessor& processor) : processor_(processor)
    {
        addAndMakeVisible(panel_);
        // panel_ is a plain paint()-only shell with no interactivity of its own
        // (SectionPanel's own doc comment) -- without this, its full-bounds hit
        // region would swallow every click in the pill row before this
        // component's own mouseDown() ever saw it. `true` for the children
        // parameter keeps the knobs (panel_'s own children) fully clickable.
        panel_.setInterceptsMouseClicks(false, true);
        showNode(0);
    }

    juce::Rectangle<int> OperatorEditorPanel::pillRowBounds() const
    {
        auto content = panel_.getContentBounds();
        content.removeFromBottom(kNoteHeight);
        return content.removeFromTop(kPillRowHeight);
    }

    juce::Rectangle<int> OperatorEditorPanel::pillBounds(int engineIndex) const
    {
        auto row = pillRowBounds();
        const int pillWidth = row.getWidth() / kNumEngines;
        return row.withX(row.getX() + pillWidth * engineIndex).withWidth(pillWidth).reduced(2, 3);
    }

    void OperatorEditorPanel::showNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, static_cast<int>(pw8::core::kNodesPerLayer) - 1, nodeIndex);
        panel_.setTitle("Operator " + juce::String(selectedNode_));

        auto& apvts = processor_.apvts;
        waveformKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "Waveform"),
                                                     "Wave", waveformToText);
        levelKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "Level"),
                                                  "Level");
        ratioKnob_ = std::make_unique<GlowKnob>(apvts, operatorParamId(static_cast<std::size_t>(selectedNode_), "FreqRatio"),
                                                  "Ratio");
        for (auto* k : {waveformKnob_.get(), levelKnob_.get(), ratioKnob_.get()})
            panel_.addAndMakeVisible(*k);

        resized();
        repaint();
    }

    void OperatorEditorPanel::mouseDown(const juce::MouseEvent& event)
    {
        const auto pos = event.getPosition();
        for (int i = 0; i < kNumEngines; ++i)
        {
            if (!pillBounds(i).contains(pos))
                continue;

            const auto engine = engineForIndex(i);
            if (!algorithm::isEngineImplemented(engine))
                return; // Honest no-op -- see the header doc comment on why this stays disabled, not hidden.

            auto& apvts = processor_.apvts;
            const auto paramId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
            if (auto* param = apvts.getParameter(paramId))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(i)));
            repaint();
            return;
        }
    }

    void OperatorEditorPanel::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds();
        content.removeFromTop(kPillRowHeight);
        content.removeFromBottom(kNoteHeight);

        const int knobWidth = content.getWidth() / 3;
        if (waveformKnob_)
            waveformKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        if (levelKnob_)
            levelKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        if (ratioKnob_)
            ratioKnob_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
    }

    void OperatorEditorPanel::paintOverChildren(juce::Graphics& g)
    {
        auto& apvts = processor_.apvts;
        const auto engineParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        const int currentEngine =
            apvts.getRawParameterValue(engineParamId) != nullptr
                ? static_cast<int>(apvts.getRawParameterValue(engineParamId)->load())
                : 0;

        for (int i = 0; i < kNumEngines; ++i)
        {
            const auto bounds = pillBounds(i).toFloat();
            const auto engine = engineForIndex(i);
            const bool implemented = algorithm::isEngineImplemented(engine);
            const bool isCurrent = i == currentEngine;

            g.setColour(isCurrent ? palette::kAccentDim : palette::kPanelRaised);
            g.fillRoundedRectangle(bounds, 5.0f);
            g.setColour(isCurrent ? palette::kAccent : palette::kBorder);
            g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);

            g.setColour(!implemented ? palette::kTextDim : (isCurrent ? palette::kTextPrimary : palette::kTextSecondary));
            g.setFont(fonts::label(9.5f));
            g.drawText(engineShortName(engine), bounds, juce::Justification::centred);
        }

        // A reserved note line under the knobs -- only filled in when the
        // currently-selected operator is running an unimplemented engine, so a
        // player who picks (or inherits, from a hand-authored .pw8) one of those 6
        // engines is told plainly why it's silent rather than left to guess.
        if (!algorithm::isEngineImplemented(engineForIndex(currentEngine)))
        {
            auto content = panel_.getContentBounds();
            auto noteRow = content.removeFromBottom(kNoteHeight).toFloat();
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(9.5f));
            g.drawText("This engine renders silence today -- not yet implemented.", noteRow,
                       juce::Justification::centredLeft);
        }
    }

} // namespace pw8::plugin::ui
