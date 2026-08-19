// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/Untitled?node-id=4-4
// source=plugin/src/ui/components/EngineCard.h
// component=EngineCard
import figma from 'figma'

export default {
  id: 'engine-card',
  imports: [
    '#include "EngineCard.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
EngineCard engineCard_(processor_, engineIndex);
addAndMakeVisible(engineCard_);
engineCard_.setBounds(cardBounds);
`,
  metadata: {
    nestable: true,
    props: {
      engineIndex: figma.number(0),
    },
  },
}
