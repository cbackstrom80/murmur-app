#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Real Sub Anchor metering readout -- a horizontal bidirectional
    /// correlation bar (same visual language as QuasarSegmentedMeter's
    /// columns, real values via MurmurProcessor::getSubAnchorCorrelation())
    /// plus a real numeric level-in-dB readout
    /// (MurmurProcessor::getSubAnchorLevelDb()). Deliberately NOT labeled
    /// "LUFS" -- real ITU-R BS.1770 K-weighting is a full-range perceptual
    /// correction; applying it to a signal Sub Anchor has already lowpassed
    /// wouldn't produce a meaningful LUFS number. This is real windowed
    /// RMS-in-dBFS of the real anchored sub-band, labeled honestly.
    ///
    /// Self-timed at 30Hz (same real pattern GlowKnob itself already uses,
    /// `private juce::Timer`) rather than requiring an external tick call
    /// from FilterLfoPanel's parent -- FilterLfoPanel has no existing
    /// periodic update hook to piggyback on today.
    class SubAnchorMeter
        : public juce::Component,
          private juce::Timer
    {
    public:
        explicit SubAnchorMeter(MurmurProcessor& processor);
        ~SubAnchorMeter() override;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        MurmurProcessor& processor_;
        float correlationDisplay_ = 1.0f;
        float levelDbDisplay_ = -100.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubAnchorMeter)
    };

} // namespace pw8::plugin::ui
