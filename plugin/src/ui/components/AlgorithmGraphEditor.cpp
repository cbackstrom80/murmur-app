#include "AlgorithmGraphEditor.h"

#include "AlgorithmGraphView.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "EdgeIconGrid.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::String edgeTypeLabel(algorithm::EdgeType type)
        {
            switch (type)
            {
                case algorithm::EdgeType::Audio: return "AUDIO";
                case algorithm::EdgeType::PhaseMod: return "PM";
                case algorithm::EdgeType::FrequencyMod: return "FM";
                case algorithm::EdgeType::AmplitudeMod: return "AM";
                case algorithm::EdgeType::RingMod: return "RING";
                case algorithm::EdgeType::Sync: return "SYNC";
                case algorithm::EdgeType::Feedback: return "FB";
            }
            return "?";
        }

        algorithm::EdgeType edgeTypeFromIndex(int index)
        {
            return static_cast<algorithm::EdgeType>(juce::jlimit(0, 6, index));
        }
    } // namespace

    class AlgorithmGraphEditor::EdgeRow : public juce::Component
    {
    public:
        std::function<void()> onChanged;
        std::function<void()> onRemove;

        EdgeRow()
        {
            for (auto* box : {&sourceBox_, &destBox_, &typeBox_})
            {
                box->setColour(juce::ComboBox::backgroundColourId, palette::kPanelRaised);
                box->setColour(juce::ComboBox::textColourId, palette::kTextPrimary);
                box->setColour(juce::ComboBox::outlineColourId, palette::kBorder);
                addAndMakeVisible(*box);
            }

            for (int i = 0; i < static_cast<int>(pw8::core::kNodesPerLayer); ++i)
            {
                sourceBox_.addItem(juce::String(i), i + 1);
                destBox_.addItem(juce::String(i), i + 1);
            }

            for (int t = 0; t <= 6; ++t)
                typeBox_.addItem(edgeTypeLabel(edgeTypeFromIndex(t)), t + 1);

            amountSlider_.setRange(0.0, 2.0, 0.001);
            amountSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 20);
            amountSlider_.setColour(juce::Slider::textBoxTextColourId, palette::kTextPrimary);
            amountSlider_.setColour(juce::Slider::textBoxBackgroundColourId, palette::kPanelRaised);
            addAndMakeVisible(amountSlider_);

            removeButton_.setButtonText("×");
            removeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            removeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
            removeButton_.onClick = [this] {
                if (onRemove)
                    onRemove();
            };
            addAndMakeVisible(removeButton_);

            auto notify = [this] {
                if (onChanged)
                    onChanged();
            };
            sourceBox_.onChange = notify;
            destBox_.onChange = notify;
            typeBox_.onChange = notify;
            amountSlider_.onValueChange = notify;
        }

        void setEdge(const algorithm::AlgorithmEdge& edge)
        {
            sourceBox_.setSelectedId(static_cast<int>(edge.source.get()) + 1, juce::dontSendNotification);
            destBox_.setSelectedId(static_cast<int>(edge.destination.get()) + 1, juce::dontSendNotification);
            typeBox_.setSelectedId(static_cast<int>(edge.type) + 1, juce::dontSendNotification);
            amountSlider_.setValue(edge.amount, juce::dontSendNotification);
        }

        [[nodiscard]] algorithm::AlgorithmEdge getEdge() const
        {
            algorithm::AlgorithmEdge edge;
            edge.source = pw8::core::NodeId(static_cast<std::uint8_t>(sourceBox_.getSelectedId() - 1));
            edge.destination = pw8::core::NodeId(static_cast<std::uint8_t>(destBox_.getSelectedId() - 1));
            edge.type = edgeTypeFromIndex(typeBox_.getSelectedId() - 1);
            edge.amount = static_cast<float>(amountSlider_.getValue());
            return edge;
        }

        void paint(juce::Graphics& g) override
        {
            const auto type = edgeTypeFromIndex(typeBox_.getSelectedId() - 1);
            auto iconArea = getLocalBounds().removeFromLeft(20).reduced(2).toFloat();
            edgeicons::drawEdgeIcon(g, type, iconArea, palette::edgeColour(static_cast<int>(type)), 1.1f);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(2);
            bounds.removeFromLeft(20);
            bounds.removeFromLeft(2);
            sourceBox_.setBounds(bounds.removeFromLeft(44));
            bounds.removeFromLeft(4);
            destBox_.setBounds(bounds.removeFromLeft(44));
            bounds.removeFromLeft(4);
            typeBox_.setBounds(bounds.removeFromLeft(72));
            bounds.removeFromLeft(4);
            amountSlider_.setBounds(bounds.removeFromLeft(120));
            bounds.removeFromLeft(4);
            removeButton_.setBounds(bounds.removeFromLeft(28));
        }

    private:
        juce::ComboBox sourceBox_;
        juce::ComboBox destBox_;
        juce::ComboBox typeBox_;
        juce::Slider amountSlider_;
        juce::TextButton removeButton_;
    };

    AlgorithmGraphEditor::AlgorithmGraphEditor(PatchworkEightProcessor& processor) : processor_(processor)
    {
        nodesHeader_.setFont(fonts::label(fonts::kBodyLabelSize));
        nodesHeader_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(nodesHeader_);

        for (std::size_t i = 0; i < outputToggles_.size(); ++i)
        {
            auto& toggle = outputToggles_[i];
            const auto& op = processor_.getCurrentPatch().layerA.operators[i];
            toggle.setButtonText(juce::String(static_cast<int>(i)) + " " + engineShortName(op.engine));
            toggle.setColour(juce::ToggleButton::textColourId, palette::kTextPrimary);
            toggle.setColour(juce::ToggleButton::tickColourId, palette::kAccent);
            toggle.setColour(juce::ToggleButton::tickDisabledColourId, palette::kTextDim);
            toggle.onClick = [this] {
                for (std::size_t n = 0; n < workingCopy_.nodes.size(); ++n)
                    workingCopy_.nodes[n].isOutput = outputToggles_[n].getToggleState();
                recompilePreview();
            };
            addAndMakeVisible(toggle);
        }

        edgesHeader_.setFont(fonts::label(fonts::kBodyLabelSize));
        edgesHeader_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        addAndMakeVisible(edgesHeader_);

        addEdgeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        addEdgeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        addEdgeButton_.onClick = [this] { addEdge(); };
        addAndMakeVisible(addEdgeButton_);

        edgeViewport_.setViewedComponent(&edgeListHost_, false);
        edgeViewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(edgeViewport_);

        compileStatus_.setFont(fonts::value(11.0f));
        compileStatus_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(compileStatus_);

        applyButton_.setColour(juce::TextButton::buttonColourId, palette::kAccentDim);
        applyButton_.setColour(juce::TextButton::textColourOffId, palette::kTextPrimary);
        applyButton_.onClick = [this] { applyEdits(); };
        addAndMakeVisible(applyButton_);

        revertButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        revertButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        revertButton_.onClick = [this] { revertEdits(); };
        addAndMakeVisible(revertButton_);

        graphPreview_ = std::make_unique<AlgorithmGraphView>(processor_);
        addAndMakeVisible(*graphPreview_);

        refreshFromPatch();
    }

    AlgorithmGraphEditor::~AlgorithmGraphEditor() = default;

    void AlgorithmGraphEditor::refreshFromPatch()
    {
        workingCopy_ = processor_.getCurrentPatch().layerA.algorithm;
        committedCopy_ = workingCopy_;
        ensureDefaultNodes();
        rebuildEdgeRows();
        recompilePreview();
    }

    void AlgorithmGraphEditor::ensureDefaultNodes()
    {
        if (workingCopy_.nodes.size() != pw8::core::kNodesPerLayer)
        {
            workingCopy_ = algorithm::AlgorithmGraphDefinition::makeDefaultParallel8();
            for (std::size_t i = 0; i < pw8::core::kNodesPerLayer; ++i)
                workingCopy_.nodes[i].engine =
                    processor_.getCurrentPatch().layerA.operators[i].engine;
        }

        for (std::size_t i = 0; i < pw8::core::kNodesPerLayer; ++i)
        {
            workingCopy_.nodes[i].engine = processor_.getCurrentPatch().layerA.operators[i].engine;
            outputToggles_[i].setToggleState(workingCopy_.nodes[i].isOutput, juce::dontSendNotification);
            outputToggles_[i].setButtonText(juce::String(static_cast<int>(i)) + " " +
                                            engineShortName(workingCopy_.nodes[i].engine));
        }
    }

    void AlgorithmGraphEditor::rebuildEdgeRows()
    {
        edgeRows_.clear();

        constexpr int kRowHeight = 30;
        edgeListHost_.setSize(edgeViewport_.getWidth(), static_cast<int>(workingCopy_.edges.size()) * kRowHeight);

        for (std::size_t i = 0; i < workingCopy_.edges.size(); ++i)
        {
            auto* row = edgeRows_.add(new EdgeRow());
            row->setEdge(workingCopy_.edges[i]);
            row->setBounds(0, static_cast<int>(i) * kRowHeight, edgeListHost_.getWidth(), kRowHeight);
            edgeListHost_.addAndMakeVisible(row);

            row->onChanged = [this] { recompilePreview(); };
            row->onRemove = [this, index = i] {
                pw8::core::FixedVector<algorithm::AlgorithmEdge, pw8::core::kMaxAlgorithmEdges> kept;
                for (std::size_t e = 0; e < workingCopy_.edges.size(); ++e)
                {
                    if (e != index)
                        kept.push_back(workingCopy_.edges[e]);
                }
                workingCopy_.edges.clear();
                for (const auto& edge : kept)
                    workingCopy_.edges.push_back(edge);
                rebuildEdgeRows();
                recompilePreview();
            };
        }
    }

    void AlgorithmGraphEditor::syncGraphPreview()
    {
        if (graphPreview_ != nullptr)
            graphPreview_->setGraphPreview(&workingCopy_);
    }

    void AlgorithmGraphEditor::recompilePreview()
    {
        workingCopy_.edges.clear();
        for (auto* row : edgeRows_)
            workingCopy_.edges.push_back(row->getEdge());

        const auto result = processor_.tryCompileAlgorithm(workingCopy_);
        lastStatus_ = result.status;

        const bool compileOk = result.ok;
        const bool dirty = !workingCopyMatchesPatch();
        compileStatus_.setText("Compile: " + result.detail, juce::dontSendNotification);
        compileStatus_.setColour(juce::Label::textColourId, compileOk ? palette::kAccent : juce::Colours::orange);
        applyButton_.setEnabled(compileOk && dirty);
        revertButton_.setEnabled(dirty);
        syncGraphPreview();
    }

    void AlgorithmGraphEditor::addEdge()
    {
        if (workingCopy_.edges.size() >= pw8::core::kMaxAlgorithmEdges)
            return;

        algorithm::AlgorithmEdge edge{};
        edge.source = pw8::core::NodeId(0);
        edge.destination = pw8::core::NodeId(1);
        edge.type = algorithm::EdgeType::PhaseMod;
        edge.amount = 0.35f;
        workingCopy_.edges.push_back(edge);
        rebuildEdgeRows();
        recompilePreview();
    }

    void AlgorithmGraphEditor::applyEdits()
    {
        if (lastStatus_ != algorithm::CompileStatus::Ok)
            return;

        workingCopy_.edges.clear();
        for (auto* row : edgeRows_)
            workingCopy_.edges.push_back(row->getEdge());

        if (!processor_.commitAlgorithmGraph(workingCopy_))
            return;

        committedCopy_ = workingCopy_;
        recompilePreview();
        if (onGraphApplied)
            onGraphApplied();
    }

    void AlgorithmGraphEditor::revertEdits()
    {
        workingCopy_ = committedCopy_;
        ensureDefaultNodes();
        rebuildEdgeRows();
        recompilePreview();
    }

    bool AlgorithmGraphEditor::workingCopyMatchesPatch() const
    {
        algorithm::AlgorithmGraphDefinition current = workingCopy_;
        current.edges.clear();
        for (auto* row : edgeRows_)
            current.edges.push_back(row->getEdge());

        for (std::size_t i = 0; i < current.nodes.size(); ++i)
            current.nodes[i].isOutput = outputToggles_[i].getToggleState();

        const auto& patchGraph = committedCopy_;
        if (patchGraph.nodes.size() != current.nodes.size())
            return false;

        for (std::size_t i = 0; i < current.nodes.size(); ++i)
        {
            if (patchGraph.nodes[i].isOutput != current.nodes[i].isOutput)
                return false;
        }

        if (patchGraph.edges.size() != current.edges.size())
            return false;

        for (std::size_t i = 0; i < current.edges.size(); ++i)
        {
            const auto& a = patchGraph.edges[i];
            const auto& b = current.edges[i];
            if (a.source.get() != b.source.get() || a.destination.get() != b.destination.get() ||
                a.type != b.type || a.amount != b.amount)
                return false;
        }

        return true;
    }

    void AlgorithmGraphEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(8);

        if (graphPreview_ != nullptr)
        {
            const int previewHeight = juce::jlimit(160, 280, bounds.getHeight() * 2 / 5);
            graphPreview_->setBounds(bounds.removeFromTop(previewHeight));
            bounds.removeFromTop(8);
        }

        nodesHeader_.setBounds(bounds.removeFromTop(20));
        auto nodeRow = bounds.removeFromTop(28);
        const int toggleWidth = nodeRow.getWidth() / static_cast<int>(outputToggles_.size());
        for (auto& toggle : outputToggles_)
            toggle.setBounds(nodeRow.removeFromLeft(toggleWidth).reduced(1));

        bounds.removeFromTop(8);
        auto edgeHeaderRow = bounds.removeFromTop(24);
        edgesHeader_.setBounds(edgeHeaderRow.removeFromLeft(80));
        addEdgeButton_.setBounds(edgeHeaderRow.removeFromRight(100).reduced(2));

        edgeViewport_.setBounds(bounds.removeFromTop(juce::jmax(120, bounds.getHeight() / 2)));
        edgeListHost_.setSize(edgeViewport_.getViewWidth(), edgeListHost_.getHeight());
        for (auto* row : edgeRows_)
            row->setSize(edgeListHost_.getWidth(), row->getHeight());

        bounds.removeFromTop(8);
        compileStatus_.setBounds(bounds.removeFromTop(22));
        bounds.removeFromTop(6);
        auto buttonRow = bounds.removeFromTop(32);
        revertButton_.setBounds(buttonRow.removeFromRight(90).reduced(2));
        applyButton_.setBounds(buttonRow.removeFromRight(140).reduced(2));
    }

} // namespace pw8::plugin::ui
