// frame=murmur-mi-ui-index (cover — use — Cover page until dedicated index lands)
// url=https://www.figma.com/design/PFt0LG6XmOiZWcSoUXIWIg/?node-id=27-1115
// source=docs/MUTABLE_INSTRUMENTS_INTEGRATION_PLAN.md
// component=—
// status=DESIGN-ONLY navigation index; dedicated murmur-mi-ui-index frame not yet named in file
import figma from 'figma'

export default {
  id: 'mi-ui-index',
  imports: [],
  example: figma.code`
// MI program cover / frame index — links to all murmur-mi-ui-* screens on Page 1.
// Runtime: no C++ surface; use Figma prototype nav or PresetBrowserOverlay patterns.
`,
  metadata: {
    nestable: false,
    props: {},
  },
}
