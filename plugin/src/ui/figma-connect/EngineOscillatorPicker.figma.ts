// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/Untitled?node-id=22-2
// source=plugin/src/ui/components/EngineOscillatorPicker.h
// component=EngineOscillatorPicker
import figma from 'figma'

export default {
  id: 'engine-oscillator-picker',
  imports: [
    '#include "EngineOscillatorPicker.h"',
    '#include "processor/MurmurProcessor.h"',
  ],
  example: figma.code`
EngineOscillatorPicker picker_(processor_, engineIndex);
addAndMakeVisible(picker_);
picker_.setBounds(pickerBounds);
`,
  metadata: {
    nestable: true,
    props: {
      engineIndex: figma.number(0),
    },
  },
}
