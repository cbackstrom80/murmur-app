// frame=morph-timeline-panel (nested in murmur-mi-ui-play-morph-timeline)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-736
// source=plugin/src/ui/components/MorphTimelineStrip.h
// component=MorphTimelineStrip
import figma from 'figma'

export default {
  id: 'mi-morph-timeline-panel',
  imports: [
    '#include "MorphTimelineStrip.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
MorphTimelineStrip morphTimeline_(processor_);
morphTimeline_.onKeyframeSelected = [this](std::size_t index) {
    processor_.selectMorphKeyframe(index);
};
addAndMakeVisible(morphTimeline_);
morphTimeline_.refresh();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
