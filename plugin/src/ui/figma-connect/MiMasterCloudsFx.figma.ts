// frame=murmur-mi-ui-master-clouds-fx
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-4435
// source=plugin/src/ui/components/DesignFxPanel.h
// component=DesignFxPanel
import figma from 'figma'

export default {
  id: 'mi-master-clouds-fx',
  imports: [
    '#include "DesignFxPanel.h"',
    '#include "DesignFxHeroViz.h"',
    '#include "ModAssignmentController.h"',
  ],
  example: figma.code`
DesignFxPanel fxPanel_(processor_, assignmentController_);
fxPanel_.setEmbeddedInDesignMode(true);
addAndMakeVisible(fxPanel_);
// Clouds granular hero on MASTER FX slots only — GRANULAR | PITCH-SHIFT | REVERB mode chips.
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
