// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/MURMUR-Obsidian?node-id=10-126
// source=plugin/src/ui/components/TopologyGraphOverlay.h
// component=TopologyGraphOverlay
import figma from 'figma'

export default {
  id: 'topology-graph-overlay',
  imports: [
    '#include "TopologyGraphOverlay.h"',
    '#include "AlgorithmGraphView.h"',
  ],
  example: figma.code`
TopologyGraphOverlay topologyOverlay_(processor_);
topologyOverlay_.onNodeSelected = [this](int node) { selectOperator(node); };
topologyOverlay_.showOverlay(selectedNodeIndex_);
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
