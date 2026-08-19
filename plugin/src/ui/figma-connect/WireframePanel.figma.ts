// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=12-34
// source=plugin/src/ui/components/WireframePanel.h
// component=WireframePanel
import figma from 'figma'

const instance = figma.selectedInstance
const kind = instance.getEnum('Kind', {
  Filter: 'Filter',
  LFO: 'LFO',
  FX: 'FX',
})

export default {
  id: 'wireframe-panel',
  imports: ['#include "WireframePanel.h"'],
  example: figma.code`
WireframePanel ${kind.toLowerCase()}Wireframe_{"${kind.toUpperCase()}", palette::kAccent};
addAndMakeVisible(${kind.toLowerCase()}Wireframe_);
`,
  metadata: {
    nestable: true,
    props: { kind },
  },
}
