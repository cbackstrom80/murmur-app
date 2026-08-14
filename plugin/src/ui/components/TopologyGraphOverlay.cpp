#include "TopologyGraphOverlay.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    TopologyGraphOverlay::TopologyGraphOverlay(PatchworkEightProcessor& processor) : processor_(processor)
    {
        graphView_ = std::make_unique<AlgorithmGraphView>(processor_);
        graphView_->onNodeSelected = [this](int node) {
            if (onNodeSelected)
                onNodeSelected(node);
        };
        addAndMakeVisible(*graphView_);

        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kTextPrimary);
        closeButton_.onClick = [this] { dismiss(); };
        addAndMakeVisible(closeButton_);

        setVisible(false);
        setInterceptsMouseClicks(true, true);
    }

    void TopologyGraphOverlay::showOverlay(int selectedNode)
    {
        graphView_->setSelectedNode(selectedNode);
        graphView_->setActiveOperator(selectedNode);
        setVisible(true);
        toFront(true);
        resized();
        repaint();
    }

    void TopologyGraphOverlay::dismiss()
    {
        setVisible(false);
        if (onDismissed)
            onDismissed();
    }

    bool TopologyGraphOverlay::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

    void TopologyGraphOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colours::black.withAlpha(0.72f));
    }

    void TopologyGraphOverlay::mouseDown(const juce::MouseEvent& event)
    {
        if (!graphView_->getBounds().contains(event.getPosition()))
            dismiss();
    }

    void TopologyGraphOverlay::resized()
    {
        auto bounds = getLocalBounds();
        const int graphSize = juce::jmin(bounds.getWidth() - 48, bounds.getHeight() - 80, 560);
        graphView_->setBounds(bounds.withSizeKeepingCentre(graphSize, graphSize + 46));
        closeButton_.setBounds(bounds.removeFromTop(40).removeFromRight(100).reduced(8, 6));
    }

} // namespace pw8::plugin::ui
