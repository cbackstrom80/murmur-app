// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=10-614
// source=plugin/src/ui/components/LiveTopologyStrip.h
// component=LiveTopologyStrip
import figma from 'figma'

export default {
  id: 'live-topology-strip',
  imports: ['#include "LiveTopologyStrip.h"'],
  example: figma.code`
LiveTopologyStrip topologyStrip_;
topologyStrip_.onTap = [this]() { topologyOverlay_.showOverlay(currentNode_); };
addAndMakeVisible(topologyStrip_);
`,
  metadata: {
    nestable: true,
    props: {},
  },
}
