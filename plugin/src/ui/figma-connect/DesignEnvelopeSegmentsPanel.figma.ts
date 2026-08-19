// frame=murmur-mi-ui-design-envelope-segments
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-2712
// source=plugin/src/ui/components/DesignEnvelopeSegmentsPanel.h (planned — Track D)
// component=DesignEnvelopeSegmentsPanel
import figma from 'figma'

export default {
  id: 'mi-design-envelope-segments',
  imports: [
    '#include "DesignEnvelopeSegmentsPanel.h"',
    '#include "ObsidianEnvelopeVisualizer.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
DesignEnvelopeSegmentsPanel envSegments_(processor_, envIndex);
envSegments_.onClosed = [this] { designEditor_.showEnginePage(); };
addAndMakeVisible(envSegments_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
