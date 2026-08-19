// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=7-40
// source=plugin/src/ui/components/FxChainStrip.h
// component=FxChainStrip
import figma from 'figma'

export default {
  id: 'fx-chain-strip',
  imports: [
    '#include "FxChainStrip.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
FxChainStrip fxChain_(processor_);
addAndMakeVisible(fxChain_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
