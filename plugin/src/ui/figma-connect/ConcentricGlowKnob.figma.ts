// Figma Code Connect — ConcentricGlowKnob (UX-09 double-ring control)
// source=plugin/src/ui/components/ConcentricGlowKnob.h
// component=ConcentricGlowKnob

import figma from "@figma/code-connect";

figma.connect(
  "ConcentricGlowKnob",
  {
    example: (props) => `
ConcentricGlowKnob ${props.instanceName}(
    apvts,
    ${props.innerParam},
    ${props.outerParam},
    "${props.innerLabel}",
    "${props.outerLabel}");
${props.instanceName}.setMaxDialDiameter(${props.diameter ?? 44});
`,
    props: {
      instanceName: figma.string("Instance Name"),
      innerParam: figma.string("Inner Param ID"),
      outerParam: figma.string("Outer Param ID"),
      innerLabel: figma.string("Inner Label"),
      outerLabel: figma.string("Outer Label"),
      diameter: figma.enum("Size", {
        Card: "44",
        Panel: "72",
        Lab: "88",
      }),
    },
  },
  {
    imports: ['#include "ConcentricGlowKnob.h"'],
  }
);
