// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=35-4
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
  },
}
