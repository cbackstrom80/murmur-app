// Real JUCE plugin entry point for the Fathom binary.

#include "FathomProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pw8::fathom::FathomProcessor();
}
