// frame=murmur-design-mod-matrix
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=27-265
// layoutJson=figma-connect/layouts/murmur-design-mod-matrix.27-265.layout.json
// source=plugin/src/ui/components/DesignModMatrixPanel.h
// component=DesignModMatrixPanel
import figma from 'figma'

export default {
  id: 'murmur-design-mod-matrix',
  imports: [
    '#include "DesignModMatrixPanel.h"',
    '#include "ModAssignmentController.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
DesignModMatrixPanel modMatrixPanel_(processor_, assignmentController_);
modMatrixPanel_.setEmbeddedInDesignMode(true);
addAndMakeVisible(modMatrixPanel_);
// Full-page 11×8 interconnect grid + active routes + quick-config sidebar.
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-design-mod-matrix.27-265.layout.json',
  },
}
