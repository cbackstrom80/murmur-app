// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/Untitled?node-id=4-1267
// source=plugin/src/ui/components/ArpPanelOverlay.h
// component=ArpPanelOverlay
import figma from 'figma'

export default {
  id: 'arp-panel-overlay',
  imports: [
    '#include "ArpPanelOverlay.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
ArpPanelOverlay arpPanelOverlay_(processor_);
arpPanelOverlay_.onClosed = [this] { closeArpPanel(); };
addChildComponent(arpPanelOverlay_);
arpPanelOverlay_.setBounds(getLocalBounds());
arpPanelOverlay_.showDrawer();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
