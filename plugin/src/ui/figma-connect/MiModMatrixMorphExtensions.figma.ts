// frame=murmur-mi-ui-mod-matrix-morph-extensions
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-1259
// source=plugin/src/ui/components/DesignModMatrixPanel.h
// component=DesignModMatrixPanel
import figma from 'figma'

export default {
  id: 'mi-mod-matrix-morph-extensions',
  imports: [
    '#include "DesignModMatrixPanel.h"',
    '#include "ModAssignmentController.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
DesignModMatrixPanel modMatrixPanel_(processor_, assignmentController_);
modMatrixPanel_.setEmbeddedInDesignMode(true);
modMatrixPanel_.onClosed = [this] { designEditor_.showEnginePage(); };
addAndMakeVisible(modMatrixPanel_);
modMatrixPanel_.showOverlay();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
