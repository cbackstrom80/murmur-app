#include "QuasarTelemetryBar.h"

#include "../../PerformanceMetricsUi.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    QuasarTelemetryBar::QuasarTelemetryBar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        for (auto* label : {&cpuLabel_, &latencyLabel_, &grainsLabel_})
        {
            label->setFont(fonts::label(9.0f));
            label->setColour(juce::Label::textColourId, palette::kTextDim);
            label->setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(*label);
        }

        tick();
    }

    void QuasarTelemetryBar::tick()
    {
        const auto metrics = readPerformanceMetrics(processor_);
        cpuLabel_.setText("CPU " + formatCpuPercent(metrics.cpuPercent), juce::dontSendNotification);
        latencyLabel_.setText("LATENCY — ms", juce::dontSendNotification);
        grainsLabel_.setText("GRAINS 0", juce::dontSendNotification);
        repaint();
    }

    void QuasarTelemetryBar::paint(juce::Graphics& g)
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
    }

    void QuasarTelemetryBar::resized()
    {
        auto bounds = getLocalBounds().reduced(4, 6);
        const int colW = bounds.getWidth() / 3;
        cpuLabel_.setBounds(bounds.removeFromLeft(colW));
        latencyLabel_.setBounds(bounds.removeFromLeft(colW));
        grainsLabel_.setBounds(bounds);
    }

} // namespace pw8::plugin::ui
