// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/Untitled?node-id=27-709
// source=plugin/src/ui/components/WavetableLabPanel.h
// component=WavetableLabPanel
import figma from 'figma'

export default {
  id: 'wavetable-lab-panel',
  imports: [
    '#include "WavetableLabPanel.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
WavetableLabPanel wavetableLab_(processor_);
wavetableLab_.onClosed = [this] { closeWavetableLab(); };
addChildComponent(wavetableLab_);
wavetableLab_.showForEngine(engineIndex);
`,
  metadata: {
    nestable: false,
    props: {
      engineIndex: figma.number(0),
    },
  },
}
