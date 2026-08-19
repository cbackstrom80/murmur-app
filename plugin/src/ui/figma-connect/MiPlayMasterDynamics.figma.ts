// frame=murmur-mi-ui-play-master-dynamics
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-1798
// source=plugin/src/ui/components/MasterOutputDeck.h
// component=MasterOutputDeck
import figma from 'figma'

export default {
  id: 'mi-play-master-dynamics',
  imports: [
    '#include "MasterOutputDeck.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
MasterOutputDeck masterDeck_(processor_);
addAndMakeVisible(masterDeck_);
// Track C: Streams-style ENVELOPE | VACTROL | FOLLOWER | COMPRESSOR modes (Sprint 4 shipped).
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
