// Figma Code Connect — TripleGlowKnob (UX-09 triple-ring ADSR / timing control)
// source=plugin/src/ui/components/TripleGlowKnob.h
// component=TripleGlowKnob

import figma from "@figma/code-connect";

figma.connect(
  "TripleGlowKnob",
  {
    example: (props) => `
TripleGlowKnob ${props.instanceName}(
    apvts,
    envelopeParamId(engineIndex, "Attack"),
    envelopeParamId(engineIndex, "Decay"),
    envelopeParamId(engineIndex, "Sustain"),
    "ATK", "DEC", "SUS");
${props.instanceName}.setMaxDialDiameter(${props.diameter ?? 72});
`,
    props: {
      instanceName: figma.string("Instance Name"),
      diameter: figma.enum("Size", {
        Large: "120",
        Medium: "88",
        Compact: "72",
      }),
    },
  },
  {
    imports: ['#include "TripleGlowKnob.h"', '#include "state/PluginState.h"'],
  }
);
