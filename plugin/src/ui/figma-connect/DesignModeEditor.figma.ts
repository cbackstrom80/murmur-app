// frame=murmur-design-engine
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=37-787
// layoutJson=figma-connect/layouts/murmur-design-engine.37-787.layout.json
// source=plugin/src/ui/DesignModeEditor.h
// component=DesignModeEditor
import figma from 'figma'

export default {
  id: 'murmur-design-engine',
  imports: [
    '#include "DesignModeEditor.h"',
    '#include "components/MasterEnvelopePanel.h"',
    '#include "components/EngineGridPanel.h"',
    '#include "components/MurmurChromeBar.h"',
  ],
  example: figma.code`
DesignModeEditor designEditor_(processor_, assignmentController_);
addAndMakeVisible(designEditor_);
// ENGINE sub-page: master-envelope-panel (82:4) + 8×220px engine cards + status bar.
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-design-engine.37-787.layout.json',
  },
}
