// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=9-96
// source=plugin/src/ui/components/GlobalPanel.h
// component=GlobalPanel
import figma from 'figma'

export default {
  id: 'global-panel',
  imports: [
    '#include "GlobalPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
GlobalPanel globalPanel_(processor_);
addAndMakeVisible(globalPanel_);
// Amber accent SectionPanel · CHAIN / OUTPUT sub-tabs
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
