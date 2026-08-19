// frame=murmur-master-motion-lab
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=94-4715
// source=plugin/src/ui/components/MasterMotionLabPanel.h
// component=MasterMotionLabPanel
import figma from 'figma'

export default {
  id: 'master-motion-lab',
  imports: [
    '#include "MasterMotionLabPanel.h"',
    '#include "MasterEnvelopePanel.h"',
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
// Ben spec: master env0 hero + 4 master LFOs — see docs/BEN_MASTER_MOTION_SPEC.md
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
