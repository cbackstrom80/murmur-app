// frame=murmur-basic-view
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=86-4
// layoutJson=figma-connect/layouts/murmur-basic-view.86-4.layout.json
// source=plugin/src/ui/PlayModeEditor.h
// component=PlayModeEditor
import figma from 'figma'

export default {
  id: 'murmur-basic-view',
  imports: [
    '#include "PlayModeEditor.h"',
    '#include "components/MasterEnvelopePanel.h"',
    '#include "components/PatchFocusPanel.h"',
    '#include "components/MurmurChromeBar.h"',
  ],
  example: figma.code`
PlayModeEditor playEditor_(processor_, assignmentController_);
playEditor_.setViewMode(layout::PlayViewMode::Basic);
addAndMakeVisible(playEditor_);
// BASIC view: master envelope hero, portamento, 4 macros, VU meters — chrome BASIC tab active.
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-basic-view.86-4.layout.json',
  },
}
