// frame=murmur-mi-ui-play-morph-timeline
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-641
// source=plugin/src/ui/components/MasterMotionLabPanel.h
// component=MasterMotionLabPanel
import figma from 'figma'

export default {
  id: 'mi-play-morph-timeline',
  imports: [
    '#include "MasterMotionLabPanel.h"',
    '#include "MorphTimelineStrip.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
MasterMotionLabPanel motionLab_(processor_);
motionLab_.onClosed = [this] { closeMotionLab(); };
motionLab_.onOpenModMatrix = [this] { openDesignModMatrix(); };
motionLab_.onOpenMorphEditor = [this] { openMorphEditor(); };
addChildComponent(motionLab_);
motionLab_.showOverlay();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
