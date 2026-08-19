// frame=murmur-mi-ui-design-utility-peaks
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-4076
// source=plugin/src/ui/components/DesignUtilityPeaksPanel.h (planned — Track F)
// component=DesignUtilityPeaksPanel
import figma from 'figma'

export default {
  id: 'mi-design-utility-peaks',
  imports: [
    '#include "DesignUtilityPeaksPanel.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
DesignUtilityPeaksPanel utilityPeaks_(processor_);
utilityPeaks_.onClosed = [this] { designEditor_.showEnginePage(); };
addAndMakeVisible(utilityPeaks_);
// Mini envelope + mini LFO utility cards only — no drum mode (product decision).
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
