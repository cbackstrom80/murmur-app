#include "TopologyGraphOverlay.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::Rectangle<int> centeredAspectPanel(juce::Rectangle<int> area, float fillRatio, double aspectRatio)
        {
            const int maxW = static_cast<int>(static_cast<float>(area.getWidth()) * fillRatio);
            const int maxH = static_cast<int>(static_cast<float>(area.getHeight()) * fillRatio);

            int panelW = maxW;
            int panelH = static_cast<int>(static_cast<double>(panelW) / aspectRatio);
            if (panelH > maxH)
            {
                panelH = maxH;
                panelW = static_cast<int>(static_cast<double>(panelH) * aspectRatio);
            }

            return area.withSizeKeepingCentre(panelW, panelH);
        }
    } // namespace

    TopologyGraphOverlay::TopologyGraphOverlay(PatchworkEightProcessor& processor) : processor_(processor)
    {
        graphView_ = std::make_unique<AlgorithmGraphView>(processor_);
        graphView_->onNodeSelected = [this](int node) {
            if (onNodeSelected)
                onNodeSelected(node);
        };

        badgeLabel_.setText("UX-09", juce::dontSendNotification);
        badgeLabel_.setFont(fonts::label(9.0f));
        badgeLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        badgeLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(badgeLabel_);

        titleLabel_.setText("LIVE TOPOLOGY", juce::dontSendNotification);
        titleLabel_.setFont(fonts::title(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        panel_.addAndMakeVisible(*graphView_);
        addAndMakeVisible(panel_);

        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
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
        grabKeyboardFocus();
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

        auto panelBounds = panel_.getBounds().toFloat();
        draw::fillRecessedRoundedRect(g, panelBounds, 10.0f);
        draw::strokeGlowPath(g, draw::roundedRectPath(panelBounds, 10.0f), 0.5f, 1.1f, false);

        auto badge = badgeLabel_.getBounds().toFloat().expanded(4.0f, 2.0f);
        g.setColour(palette::kAccent.withAlpha(0.13f));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(palette::kAccent);
        g.drawRoundedRectangle(badge, 4.0f, 1.0f);
    }

    void TopologyGraphOverlay::mouseDown(const juce::MouseEvent& event)
    {
        if (!panel_.getBounds().contains(event.getPosition()))
            dismiss();
    }

    void TopologyGraphOverlay::resized()
    {
        auto bounds = getLocalBounds();
        auto header = bounds.removeFromTop(40);
        closeButton_.setBounds(header.removeFromRight(120).reduced(4, 6));
        badgeLabel_.setBounds(header.removeFromLeft(40).withSizeKeepingCentre(36, 18));
        header.removeFromLeft(8);
        titleLabel_.setBounds(header);

        panel_.setBounds(centeredAspectPanel(bounds, layout::kOverlayFillRatio, layout::kAspectRatio));
        graphView_->setBounds(panel_.getContentBounds().reduced(6));
    }

} // namespace pw8::plugin::ui
