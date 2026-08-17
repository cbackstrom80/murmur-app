// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=2-87
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
