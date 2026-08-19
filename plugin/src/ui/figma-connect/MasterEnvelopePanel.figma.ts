// frame=master-envelope-panel
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=82-4
// layoutJson=figma-connect/layouts/master-envelope-panel.82-4.layout.json
// source=plugin/src/ui/components/MasterEnvelopePanel.h
// component=MasterEnvelopePanel
import figma from 'figma'

export default {
  id: 'master-envelope-panel',
  imports: [
    '#include "MasterEnvelopePanel.h"',
    '#include "ObsidianEnvelopeVisualizer.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
MasterEnvelopePanel masterEnvelopePanel_(processor_);
addAndMakeVisible(masterEnvelopePanel_);
// Embedded on murmur-design-engine (37:787) above engine grid — 1248×320.
`,
  metadata: {
    nestable: true,
    props: {},
    layoutSpec: 'layouts/master-envelope-panel.82-4.layout.json',
  },
}
