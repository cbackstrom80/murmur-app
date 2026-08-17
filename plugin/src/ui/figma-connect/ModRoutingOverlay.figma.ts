// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=4-74
// source=plugin/src/ui/components/ModRoutingOverlay.h
// component=ModRoutingOverlay
import figma from 'figma'

export default {
  id: 'mod-routing-overlay',
  imports: [
    '#include "ModRoutingOverlay.h"',
    '#include "ModAssignmentController.h"',
  ],
  example: figma.code`
ModRoutingOverlay modOverlay_(processor_, assignmentController_);
modOverlay_.onClosed = [this]() { hideModOverlay(); };
modOverlay_.showOverlay();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
