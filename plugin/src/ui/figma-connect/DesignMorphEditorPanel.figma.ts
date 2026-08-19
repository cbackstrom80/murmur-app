// frame=murmur-mi-ui-design-morph-editor
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=89-953
// source=plugin/src/ui/components/DesignMorphEditorPanel.h
// component=DesignMorphEditorPanel
import figma from 'figma'

export default {
  id: 'mi-design-morph-editor',
  imports: [
    '#include "DesignMorphEditorPanel.h"',
    '#include "processor/PatchworkEightProcessor.h"',
  ],
  example: figma.code`
DesignMorphEditorPanel morphPanel_(processor_);
morphPanel_.onClosed = [this] { designEditor_.showEnginePage(); };
morphPanel_.onOpenModMatrix = [this] { openDesignModMatrix(); };
morphPanel_.setEmbeddedInDesignMode(true);
addAndMakeVisible(morphPanel_);
morphPanel_.refreshFromPatch();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
