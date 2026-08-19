// frame=murmur-compact-view (canonical: murmur-play-compact)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=4-1134
// layoutJson=figma-connect/layouts/murmur-compact-view.4-1134.layout.json
// source=plugin/src/ui/components/CompactModeEditor.cpp
// component=CompactModeEditor
import figma from 'figma'

export default {
  id: 'murmur-play-compact',
  imports: [
    '#include "components/CompactModeEditor.h"',
    '#include "components/MurmurChromeBar.h"',
    '#include "components/OscilloscopeView.h"',
  ],
  example: figma.code`
CompactModeEditor compactEditor_(processor_);
addAndMakeVisible(compactEditor_);
// Figma 4:1134 — 320×560 shell (header 28px → MurmurChromeBar, body → CompactModeEditor).
`,
  metadata: {
    nestable: false,
    figmaNodeId: '4:1134',
    frameSize: { width: 320, height: 560 },
    layoutSpec: 'layouts/murmur-compact-view.4-1134.layout.json',
    props: {},
  },
}
