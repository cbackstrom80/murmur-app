// frame=master-envelope-section
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=82-83
// source=plugin/src/ui/components/MasterEnvelopePanel.h
// component=MasterEnvelopePanel
import figma from 'figma'

export default {
  id: 'master-envelope-section',
  imports: [
    '#include "MasterEnvelopePanel.h"',
    '#include "MasterMotionLabPanel.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
// Compact hero slice inside murmur-mi-ui-play-morph-timeline (89:641).
MasterMotionLabPanel motionLab_(processor_);
addChildComponent(motionLab_);
motionLab_.showOverlay();
// Child region master-envelope-section (82:83) — 1240×220 ADSR curve + row knobs.
`,
  metadata: {
    nestable: true,
    props: {},
  },
}
