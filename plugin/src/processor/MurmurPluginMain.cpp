// Real JUCE plugin entry point for the MURMUR binary. Split out of
// MurmurProcessor.cpp so that file can be reused by reference (see
// docs/UNDERTOW.md, real relative-path `target_sources()` reuse, not a
// fork) by Undertow's own separate `pw8_undertow_plugin` target without
// also inheriting MURMUR's factory function -- JUCE requires exactly one
// `createPluginFilter()` definition per plugin binary, and Undertow
// provides its own (UndertowPluginMain.cpp) constructing the same real
// MurmurProcessor with a different ProductIdentity.

#include "MurmurProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pw8::plugin::MurmurProcessor();
}
