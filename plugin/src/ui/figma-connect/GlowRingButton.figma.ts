// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=2-71
// source=plugin/src/ui/components/GlowRingButton.h
// component=GlowRingButton
import figma from 'figma'

const instance = figma.selectedInstance
const label = instance.getString('Label') ?? 'PLAY'

export default {
  id: 'glow-ring-button',
  imports: ['#include "GlowRingButton.h"'],
  example: figma.code`
GlowRingButton ${label.toLowerCase()}Button_{"${label}"};
addAndMakeVisible(${label.toLowerCase()}Button_);
`,
  metadata: {
    nestable: true,
    props: { label },
  },
}
