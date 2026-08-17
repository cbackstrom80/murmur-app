// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/Untitled?node-id=15-4
// source=plugin/src/ui/components/VocoderLabPanel.h
// component=VocoderLabPanel
import figma from 'figma'

export default {
  id: 'vocoder-lab-panel',
  imports: [
    '#include "VocoderLabPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
VocoderLabPanel vocoderLab_(processor_);
vocoderLab_.onClosed = [this] { closeVocoderLab(); };
addChildComponent(vocoderLab_);
vocoderLab_.showForFxSlot(slotIndex);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
