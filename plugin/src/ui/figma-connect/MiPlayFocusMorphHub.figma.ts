// frame=murmur-mi-ui-play-focus-morph-hub
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=94-5038
// source=plugin/src/ui/components/PatchFocusPanel.h
// component=PatchFocusPanel
import figma from 'figma'

export default {
  id: 'mi-play-focus-morph-hub',
  imports: [
    '#include "PatchFocusPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
PatchFocusPanel focusPanel_(processor_);
focusPanel_.setDesktopPlayModeLayout(true);
addAndMakeVisible(focusPanel_);
// EVOLVE morph KOIN card: hue ring, keyframe labels, mini timeline — tap opens MOTION tab.
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
