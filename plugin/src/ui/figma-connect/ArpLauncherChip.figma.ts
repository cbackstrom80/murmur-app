// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=9-3
// source=plugin/src/ui/components/ArpLauncherChip.h
// component=ArpLauncherChip
import figma from 'figma'

export default {
  id: 'arp-launcher-chip',
  imports: [
    '#include "ArpLauncherChip.h"',
    '#include "processor/MurmurProcessor.h"',
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
