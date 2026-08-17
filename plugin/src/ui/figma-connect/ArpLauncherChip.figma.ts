// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=9-3
// source=plugin/src/ui/components/ArpLauncherChip.h
// component=ArpLauncherChip
import figma from 'figma'

export default {
  id: 'arp-launcher-chip',
  imports: [
    '#include "ArpLauncherChip.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
ArpLauncherChip arpChip_(processor_);
arpChip_.onOpenDrawer = [this]() { arpOverlay_.showDrawer(); };
addAndMakeVisible(arpChip_);
`,
  metadata: {
    nestable: true,
    props: {},
  },
}
