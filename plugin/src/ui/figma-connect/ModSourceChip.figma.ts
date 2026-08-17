// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=4-38
// source=plugin/src/ui/components/ModSourceChip.h
// component=ModSourceChip
import figma from 'figma'

const instance = figma.selectedInstance
const source = instance.getEnum('Source', {
  LFO: 'modulation::ModSource::Lfo1',
  ENV: 'modulation::ModSource::Env1',
  Velocity: 'modulation::ModSource::Velocity',
  'Mod Wheel': 'modulation::ModSource::ModWheel',
  Expression: 'modulation::ModSource::Expression',
  Macro: 'modulation::ModSource::Macro1',
})

export default {
  id: 'mod-source-chip',
  imports: ['#include "ModSourceChip.h"'],
  example: figma.code`
ModSourceChip chip(controller_, ${source}, modSourceLabel(${source}), modSourceColour(${source}));
addAndMakeVisible(chip);
`,
  metadata: {
    nestable: true,
    props: { source },
  },
}
