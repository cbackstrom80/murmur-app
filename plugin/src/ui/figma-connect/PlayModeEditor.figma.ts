// frame=murmur-play-view (canonical: murmur-desktop-play-mode)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=36-4
// layoutJson=figma-connect/layouts/murmur-play-view.36-4.layout.json
// source=plugin/src/ui/PlayModeEditor.h
// component=PlayModeEditor
import figma from 'figma'

export default {
  id: 'murmur-desktop-play-mode',
  imports: [
    '#include "PlayModeEditor.h"',
    '#include "components/OscilloscopeView.h"',
    '#include "components/PatchFocusPanel.h"',
    '#include "components/MurmurChromeBar.h"',
  ],
  example: figma.code`
PlayModeEditor playEditor_(processor_, assignmentController_);
addAndMakeVisible(playEditor_);
// Basic PLAY: scope + 8 macro deck + bottom bar; chrome via MurmurChromeBar (39:142).
`,
  metadata: {
    nestable: false,
    props: {},
    layoutSpec: 'layouts/murmur-play-view.36-4.layout.json',
  },
}
