// frame=murmur-mi-ui-chrome-fr-step-badge
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-1763
// source=plugin/src/ui/components/MurmurChromeBar.h
// component=MurmurChromeBar
import figma from 'figma'

export default {
  id: 'mi-chrome-fr-step-badge',
  imports: [
    '#include "MurmurChromeBar.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
MurmurChromeBar chromeBar_(processor_);
chromeBar_.setEditorMode(layout::EditorMode::Play);
addAndMakeVisible(chromeBar_);
// FR.STEP badge flashes when morph play-head crosses a keyframe (see frStepFlashTicks_).
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
