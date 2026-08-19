// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=12-3
// source=plugin/src/ui/components/AlgorithmGraphView.h
// component=AlgorithmGraphView
import figma from 'figma'

export default {
  id: 'algorithm-graph-view',
  imports: [
    '#include "AlgorithmGraphView.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
AlgorithmGraphView graphView_(processor_);
graphView_.onNodeSelected = [this](int node) { focusOperator(node); };
addAndMakeVisible(graphView_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
