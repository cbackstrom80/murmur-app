// frame=murmur-design-fx
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=35-4
// layoutJson=figma-connect/layouts/murmur-design-fx.35-4.layout.json
// source=plugin/src/ui/components/DesignFxPanel.h
// component=DesignFxPanel
import figma from 'figma'

export default {
  id: 'design-fx-panel',
  imports: [
    '#include "DesignFxPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
    '#include "state/ModAssignmentController.h"',
  ],
  example: figma.code`
DesignFxPanel designFxPanel_(processor_, modAssignmentController_);
addAndMakeVisible(designFxPanel_);
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-design-fx.35-4.layout.json',
  },
}
