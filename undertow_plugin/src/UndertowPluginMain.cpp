// Real JUCE plugin entry point for the Undertow binary. Constructs the
// exact same real MurmurProcessor (same engine, same 361 params, same DSP,
// including the real Sub Anchor bass-anchoring feature -- pw8/spatial/
// SubAnchor.hpp) MURMUR uses, just with Undertow's own ProductIdentity, so
// there is no forked processor class to keep in sync (docs/UNDERTOW.md,
// contrast with QUASAR's abandoned QuasarProcessor duplicate).

#include "processor/MurmurProcessor.h"

namespace
{
    const pw8::plugin::MurmurProcessor::ProductIdentity kUndertowIdentity{
        pw8::plugin::MurmurProcessor::ProductKind::Undertow,
        "Undertow",
        "Undertow",
        "Undertow",
    };
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pw8::plugin::MurmurProcessor(kUndertowIdentity);
}
