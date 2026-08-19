// frame=murmur-fx-saturation (hero panel shared across murmur-fx-* detail frames)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=63-8
// layoutJson=figma-connect/layouts/murmur-fx-hero.63-8.layout.json
// source=plugin/src/ui/components/wireframe/DesignFxHeroViz.h
// component=DesignFxHeroViz
import figma from 'figma'

export default {
  id: 'design-fx-hero-viz',
  imports: [
    '#include "wireframe/DesignFxHeroViz.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
wireframe::DesignFxHeroViz heroViz_(processor.apvts);
heroViz_.bindChip(1, designFxEngineSlotPrefix(1), &processor, 1);
addAndMakeVisible(heroViz_);
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-fx-hero.63-8.layout.json',
  },
}
