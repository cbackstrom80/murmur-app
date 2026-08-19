// frame=murmur-mi-ui-play-filter-blades
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-5
// source=plugin/src/ui/components/FilterLfoPanel.h
// component=FilterLfoPanel
import figma from 'figma'

export default {
  id: 'mi-play-filter-blades',
  imports: [
    '#include "FilterLfoPanel.h"',
    '#include "ModAssignmentController.h"',
    '#include "wireframe/FilterRoutingWireframeView.h"',
  ],
  example: figma.code`
FilterLfoPanel filterPanel_(processor_, assignmentController_);
filterPanel_.setScope(FilterPanelScope::Global);
addAndMakeVisible(filterPanel_);
// BLADES row: ROUTE / MORPH / F2 CUTOFF / F2 RESO / DRIVE / F2 OFFSET / KEY TRK
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
