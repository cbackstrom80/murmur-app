// frame=murmur-mi-ui-component-blades-routing-diagram
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-246
// source=plugin/src/ui/components/wireframe/FilterRoutingWireframeView.h
// component=FilterRoutingWireframeView
import figma from 'figma'

export default {
  id: 'mi-blades-routing-diagram',
  imports: [
    '#include "wireframe/FilterRoutingWireframeView.h"',
    '#include "state/PluginState.h"',
  ],
  example: figma.code`
wireframe::FilterRoutingWireframeView routingDiagram_(apvts_);
addAndMakeVisible(routingDiagram_);
// Variants: serial (0) → parallel (0.5) → crossfade (1) via filterRouting APVTS.
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
