#pragma once

#include <juce_core/juce_core.h>

#include "pw8/lfo/Lfo.hpp"
#include "pw8/sequencer/ArpeggiatorTypes.hpp"

namespace pw8::plugin::ui
{
    inline juce::String arpModeToText(float v)
    {
        switch (static_cast<sequencer::ArpMode>(static_cast<int>(v)))
        {
            case sequencer::ArpMode::Up: return "UP";
            case sequencer::ArpMode::Down: return "DOWN";
            case sequencer::ArpMode::UpDown: return "UP-DOWN";
            case sequencer::ArpMode::DownUp: return "DOWN-UP";
            case sequencer::ArpMode::AsPlayed: return "AS PLAYED";
            case sequencer::ArpMode::Random: return "RANDOM";
            case sequencer::ArpMode::Chord: return "CHORD";
        }
        return juce::String(v, 0);
    }

    inline juce::String arpRateModeToText(float v)
    {
        switch (static_cast<sequencer::ArpRateMode>(static_cast<int>(v)))
        {
            case sequencer::ArpRateMode::Free: return "FREE";
            case sequencer::ArpRateMode::TempoSync: return "SYNC";
        }
        return juce::String(v, 0);
    }

    inline juce::String arpSyncDivisionLabel(int index)
    {
        static constexpr const char* kLabels[] = {"4 BAR", "2 BAR", "1 BAR", "1/2",  "1/4",  "1/8",
                                                  "1/16", "1/4T", "1/8T", "1/4."};
        const int idx = juce::jlimit(0, static_cast<int>(lfo::kSyncDivisionQuarterNotes.size()) - 1, index);
        return kLabels[idx];
    }

    inline juce::String arpSyncDivisionToText(float v)
    {
        return arpSyncDivisionLabel(static_cast<int>(v));
    }

    inline juce::String arpRateSummary(bool tempoSync, float rateHz, int syncDivisionIndex)
    {
        if (tempoSync)
            return arpSyncDivisionLabel(syncDivisionIndex);
        return juce::String(rateHz, rateHz >= 10.0f ? 1 : 2) + " Hz";
    }

} // namespace pw8::plugin::ui
