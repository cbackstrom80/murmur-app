// url=https://www.figma.com/design/pi5kUNcZWQGhqfRAVu3voh/MURMUR-Obsidian?node-id=4-39
// source=plugin/src/ui/components/PresetBrowserOverlay.h
// component=PresetBrowserOverlay
import figma from 'figma'

export default {
  id: 'preset-browser-overlay',
  imports: [
    '#include "PresetBrowserOverlay.h"',
    '#include "content/PresetIndex.h"',
    '#include "content/FavoritesStore.h"',
  ],
  example: figma.code`
PresetBrowserOverlay browser_(processor_, presetIndex_, favoritesStore_);
browser_.onClosed = [this]() { hidePresetBrowser(); };
addChildComponent(browser_);
browser_.showOverlay();
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
