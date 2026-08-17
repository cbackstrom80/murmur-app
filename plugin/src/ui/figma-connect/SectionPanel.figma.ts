// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=2-113
// source=plugin/src/ui/components/SectionPanel.h
// component=SectionPanel
import figma from 'figma'

const instance = figma.selectedInstance
const title = instance.getString('Title') ?? 'SECTION'

export default {
  id: 'section-panel',
  imports: ['#include "SectionPanel.h"'],
  example: figma.code`
SectionPanel panel_{"${title}"};
addAndMakeVisible(panel_);
// Layout child content inside panel_.getContentBounds()
`,
  metadata: {
    nestable: true,
    props: { title },
  },
}
