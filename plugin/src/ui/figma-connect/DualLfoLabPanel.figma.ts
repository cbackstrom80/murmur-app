// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/Untitled?node-id=15-247
// source=plugin/src/ui/components/DualLfoLabPanel.h
// component=DualLfoLabPanel
import figma from 'figma'

export default {
  id: 'dual-lfo-lab-panel',
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
  },
}
