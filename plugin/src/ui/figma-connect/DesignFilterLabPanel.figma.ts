// frame=murmur-mi-ui-design-filter-lab
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-313
// source=plugin/src/ui/components/DesignFilterLabPanel.h (planned — Track B-M4)
// component=DesignFilterLabPanel
import figma from 'figma'

export default {
  id: 'mi-design-filter-lab',
  imports: [
    '#include "DesignFilterLabPanel.h"',
    '#include "FilterLfoPanel.h"',
    '#include "wireframe/FilterRoutingWireframeView.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
DesignFilterLabPanel filterLab_(processor_, assignmentController_);
filterLab_.onClosed = [this] { designEditor_.showEnginePage(); };
filterLab_.onOpenModMatrix = [this] { openDesignModMatrix(); };
filterLab_.onOpenPlayFilter = [this] { switchToPlayFilterTab(); };
addAndMakeVisible(filterLab_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
