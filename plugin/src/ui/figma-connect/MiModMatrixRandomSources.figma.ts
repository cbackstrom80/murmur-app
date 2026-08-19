// frame=murmur-mi-ui-mod-matrix-random-sources
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-3911
// source=plugin/src/ui/components/DesignModMatrixPanel.h
// component=DesignModMatrixPanel
import figma from 'figma'

export default {
  id: 'mi-mod-matrix-random-sources',
  imports: [
    '#include "DesignModMatrixPanel.h"',
    '#include "ModSourceChip.h"',
  ],
  example: figma.code`
// Sidebar slice: RANDOM 1–4 chips + deja-vu / T / X generator sections (Track E).
DesignModMatrixPanel modMatrixPanel_(processor_, assignmentController_);
modMatrixPanel_.setEmbeddedInDesignMode(true);
addAndMakeVisible(modMatrixPanel_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
