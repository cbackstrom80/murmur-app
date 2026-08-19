// frame=blades-section-panel (nested in murmur-mi-ui-play-filter-blades)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-131
// source=plugin/src/ui/components/FilterLfoPanel.h
// component=FilterLfoPanel
import figma from 'figma'

export default {
  id: 'mi-blades-section-panel',
  imports: [
    '#include "FilterLfoPanel.h"',
    '#include "ModAssignmentController.h"',
  ],
  example: figma.code`
FilterLfoPanel filterPanel_(processor_, assignmentController_);
filterPanel_.setScope(FilterPanelScope::Global);
addAndMakeVisible(filterPanel_);
// BLADES section: header-row + knobs-row (7 knobs, 104px row height).
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
