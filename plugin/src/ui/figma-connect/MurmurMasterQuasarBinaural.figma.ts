// frame=murmur-master-quasar-binaural
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=102-4
// source=plugin/src/ui/components/MasterQuasarPanel.h + plugin/src/ui/components/quasar/*
// component=MasterQuasarPanel
import figma from 'figma'

export default {
  id: 'murmur-master-quasar-binaural',
  imports: [
    '#include "MasterQuasarPanel.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
MasterQuasarPanel quasarPanel_(processor_);
quasarPanel_.onBackToFxChain = [this] { showGlobalFxChain(); };
addAndMakeVisible(quasarPanel_);
// Sprint 8 — in-MURMUR only; Figma 102:4 hero binaural spatializer.
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
