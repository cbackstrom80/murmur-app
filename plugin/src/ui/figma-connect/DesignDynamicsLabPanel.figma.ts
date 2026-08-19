// frame=murmur-mi-ui-design-dynamics-lab
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-2059
// source=plugin/src/ui/components/DesignDynamicsLabPanel.h (planned — Track C)
// component=DesignDynamicsLabPanel
import figma from 'figma'

export default {
  id: 'mi-design-dynamics-lab',
  imports: [
    '#include "DesignDynamicsLabPanel.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
DesignDynamicsLabPanel dynamicsLab_(processor_);
dynamicsLab_.onClosed = [this] { designEditor_.showEnginePage(); };
addAndMakeVisible(dynamicsLab_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
