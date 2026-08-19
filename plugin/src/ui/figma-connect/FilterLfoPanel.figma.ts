// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=10-44
// source=plugin/src/ui/components/FilterLfoPanel.h
// component=FilterLfoPanel
import figma from 'figma'

export default {
  id: 'filter-lfo-panel',
  imports: [
    '#include "FilterLfoPanel.h"',
    '#include "ModAssignmentController.h"',
  ],
  example: figma.code`
FilterLfoPanel filterPanel_(processor_, assignmentController_);
filterPanel_.setScope(FilterPanelScope::Global);
addAndMakeVisible(filterPanel_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
