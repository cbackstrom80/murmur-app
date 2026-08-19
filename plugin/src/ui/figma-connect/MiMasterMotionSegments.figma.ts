// frame=murmur-mi-ui-master-motion-segments
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-2381
// source=plugin/src/ui/components/MasterMotionLabPanel.h
// component=MasterMotionLabPanel
import figma from 'figma'

export default {
  id: 'mi-master-motion-segments',
  imports: [
    '#include "MasterMotionLabPanel.h"',
    '#include "MasterEnvelopePanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
MasterMotionLabPanel motionLab_(processor_);
addAndMakeVisible(motionLab_);
// Stages-style segment strip replaces single ADSR hero on MasterEnvelopePanel (Track D).
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
