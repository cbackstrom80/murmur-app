// frame=murmur-dual-lfo-lab
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=15-247
// layoutJson=figma-connect/layouts/murmur-dual-lfo-lab.15-247.layout.json
// source=plugin/src/ui/components/DualLfoLabPanel.h
// component=DualLfoLabPanel
import figma from 'figma'

export default {
  id: 'murmur-dual-lfo-lab',
  imports: [
    '#include "DualLfoLabPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
DualLfoLabPanel dualLfoLab_(processor_);
dualLfoLab_.onClosed = [this] { closeDualLfoLab(); };
dualLfoLab_.onOpenModMatrix = [this] { openModRoutingOverlay(); };
addChildComponent(dualLfoLab_);
dualLfoLab_.showOverlay();
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-dual-lfo-lab.15-247.layout.json',
  },
}
