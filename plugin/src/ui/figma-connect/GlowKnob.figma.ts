// frame=glow-ring-knobs
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=21-4
// layoutJson=figma-connect/layouts/glow-ring-knobs.21-4.layout.json
// source=plugin/src/ui/components/GlowKnob.h
// component=GlowKnob
import figma from 'figma'

const instance = figma.selectedInstance
const label = instance.getString('Label') ?? 'CUTOFF'

export default {
  id: 'glow-knob',
  imports: [
    '#include "GlowKnob.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
GlowKnob ${label.toLowerCase()}Knob(apvts_, "param.${label.toLowerCase()}", "${label}");
addAndMakeVisible(${label.toLowerCase()}Knob);
`,
  metadata: {
    nestable: true,
    props: { label },
  },
}
