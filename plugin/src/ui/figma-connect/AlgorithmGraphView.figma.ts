// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=12-3
// source=plugin/src/ui/components/AlgorithmGraphView.h
// component=AlgorithmGraphView
import figma from 'figma'

export default {
  id: 'algorithm-graph-view',
  imports: [
    '#include "AlgorithmGraphView.h"',
    '#include "processor/PatchworkEightProcessor.h"',
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
