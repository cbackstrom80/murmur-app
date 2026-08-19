// frame=murmur-engine-deep-editor
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=28-4
// layoutJson=figma-connect/layouts/murmur-engine-deep-editor.28-4.layout.json
// source=plugin/src/ui/components/EngineDetailOverlay.h
// component=EngineDetailOverlay
import figma from 'figma'

export default {
  id: 'murmur-engine-deep-editor',
  imports: [
    '#include "EngineDetailOverlay.h"',
    '#include "OperatorEditorPanel.h"',
    '#include "FilterLfoPanel.h"',
  ],
  example: figma.code`
EngineDetailOverlay engineDetail_(processor_, assignmentController_);
engineDetail_.onClosed = [this]() { hideEngineDetail(); };
addChildComponent(engineDetail_);
engineDetail_.showOverlay();
// Letterboxed 1440×1024 OSC / Filter / Amp deep editor.
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-engine-deep-editor.28-4.layout.json',
  },
}
