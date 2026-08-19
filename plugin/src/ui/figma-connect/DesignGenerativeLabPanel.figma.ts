// frame=murmur-mi-ui-design-generative-lab
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-3479
// source=plugin/src/ui/components/DesignGenerativeLabPanel.h (planned — Track E)
// component=DesignGenerativeLabPanel
import figma from 'figma'

export default {
  id: 'mi-design-generative-lab',
  imports: [
    '#include "DesignGenerativeLabPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
DesignGenerativeLabPanel generativeLab_(processor_);
generativeLab_.onClosed = [this] { designEditor_.showEnginePage(); };
generativeLab_.onOpenModMatrix = [this] { openDesignModMatrix(); };
addAndMakeVisible(generativeLab_);
// Canonical frame: 89:3479 (duplicate at 89:3002 — deprecated).
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
