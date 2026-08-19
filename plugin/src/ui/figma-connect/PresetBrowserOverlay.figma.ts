// frame=murmur-preset-browser (full-page 27:6; modal variant 74:959)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=27-6
// layoutJson=figma-connect/layouts/murmur-preset-browser.27-6.layout.json
// source=plugin/src/ui/components/PresetBrowserOverlay.h
// component=PresetBrowserOverlay
import figma from 'figma'

export default {
  id: 'murmur-preset-browser',
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
    layoutSpec: 'layouts/murmur-preset-browser.27-6.layout.json',
  },
}
