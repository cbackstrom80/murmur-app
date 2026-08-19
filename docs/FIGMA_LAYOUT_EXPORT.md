# Figma Layout Export — Spec-First UI Pipeline

**Problem:** Layout constants drift from Figma because metadata lives in audit markdown, Code Connect stubs are thin, and agents re-interpret numbers each session.

**Goal:** One machine-readable layout spec per frame → named C++ constants → component `resized()` — no guessing.

---

## Pipeline

```
Figma frame (e.g. 4:1134)
    │
    ▼ get_metadata (verbatim x/y/w/h — no interpretation)
    │
    ▼ scripts/figma_layout.sh export [--annotate]  →  layouts/*.layout.json
    │
    ▼ PlayModeLayout.h constants (k* ↔ JSON "constant" + optional "drift")
    │
    ▼ Component resized() reads constants only
    │
    ▼ scripts/figma_layout.sh check  (mvp_check + CI + release_gate)
```

### Rules

1. **Never copy numbers into markdown alone** — update the `.layout.json` first, then `PlayModeLayout.h`, then audit tables.
2. **Split frames explicitly** — if code splits chrome (`MurmurChromeBar`) from body (`CompactModeEditor`), JSON `codeOwners` documents the split; do not double-apply insets.
3. **Asymmetric insets are first-class** — `4:1134` uses top=14, bottom=**30**; use `BorderSize`, not single `outerMargin`.
4. **Fixed vs flex** — JSON marks fixed-height sections; only marked-flex regions may absorb slack.
5. **Documented drift** — when Figma ≠ runtime C++, use structured `drift: { cpp, reason }` on the node (not free-text `note`).
6. **Code Connect stub** — each `.figma.ts` links `layoutJson` + Figma URL + C++ owner.

---

## Toolchain

| Command | Purpose |
|---------|---------|
| `scripts/figma_layout.sh check` | **Game-time gate** — schema, constants, drift, geometry, columns, parent links, token ladder, reverse audit |
| `scripts/figma_layout_sync.sh` | `check` + unit tests (+ optional `FIGMA_METADATA_XML` compare) |
| `scripts/figma_layout.sh validate` | Constants + drift subset of `check` |
| `scripts/figma_layout.sh export …` | `get_metadata` XML → `.layout.json` (optional `--annotate`) |
| `scripts/figma_layout.sh annotate <file>` | Suggest / apply `constant`, `codeMap`, insets from policy + PlayModeLayout.h |
| `scripts/figma_layout.sh compare …` | Diff metadata XML vs JSON (`--mode bounds\|top-level`, `--tolerance N`, `--strict`) |
| `scripts/figma_layout.sh fetch <file>` | Live Figma REST bounds vs JSON (`FIGMA_ACCESS_TOKEN` required) |
| `scripts/figma_layout.sh snippet <file>` | Emit canonical-aware `PlayModeLayout.h` snippet |
| `scripts/figma_layout.sh report` | Markdown summary (tiers, pending, issues, audit rows) |
| `scripts/figma_layout.sh list` | Show `layouts/manifest.json` |

**Game-time gate** runs in:

- `scripts/mvp_check.sh`
- `scripts/release_gate.sh`
- `.github/workflows/ci.yml`
- `python3 scripts/test_figma_layout.py` (30 regression tests)

### Layout policies (declarative)

Per-frame validation rules live in `scripts/figma_layout/policies.py` (defaults) and can be overridden in `manifest.json` → `registry[].layoutPolicy`:

- `tier`: `required` | `exported` | `pending`
- `frameSize` / `frameWidthOnly`
- `sectionGap`, `insets` → PlayModeLayout constant names
- `geometry`: `vertical-stack`
- `columnLayouts`: horizontal width budgets (basic view, mod matrix)
- `parentLink`: sub-panel embed checks (master envelope)
- `validateScaleLadder` / `validateHeroRatios` (glow knobs)
- `cppOwners`: reverse audit targets

Adding frame #9 should be manifest + JSON only — not new Python branches.

### Drift model

When Figma and C++ disagree intentionally:

```json
{
  "name": "matrix-grid-card",
  "height": 306,
  "constant": "kDesignModMatrixPageGridCardHeight",
  "drift": {
    "cpp": 304,
    "reason": "DesignModMatrixPanel::removeFromTop uses C++ constant"
  }
}
```

`check` errors on **undocumented** mismatches; documented drift is allowed.

### Export + annotate workflow

```bash
# Export geometry, auto-suggest constants/codeMaps
pbpaste | scripts/figma_layout.sh export \
  --file-key PFt0LG6XmOiZWcSoUXIWIg \
  --node-id 27:265 \
  --annotate --verbose

# Refine suggestions on an existing spec
scripts/figma_layout.sh annotate murmur-basic-view.86-4.layout.json
scripts/figma_layout.sh annotate murmur-basic-view.86-4.layout.json --write
```

### Compare modes

```bash
# All bound nodes (default)
scripts/figma_layout.sh compare murmur-compact-view.4-1134.layout.json -i metadata.xml

# Top-level sections only (CI-friendly)
scripts/figma_layout.sh compare murmur-basic-view.86-4.layout.json -i metadata.xml \
  --mode top-level --tolerance 0
```

### Live Figma fetch

Store your token in **gitignored** `.env.local` (copy from `.env.local.example`). `scripts/figma_layout.sh` loads it automatically.

```bash
cp .env.local.example .env.local
# edit .env.local — Figma Settings → Security → Personal access tokens

scripts/figma_layout.sh fetch murmur-design-mod-matrix.27-265.layout.json \
  --mode top-level -o /tmp/27-265.live.xml
```

### Report

```bash
scripts/figma_layout.sh report --output docs/FIGMA_LAYOUT_REPORT.md
```

### Exported specs (game-time)

| Frame | Node | Tier | Layout JSON |
|-------|------|------|-------------|
| murmur-play-compact | `4:1134` | required | `murmur-compact-view.4-1134.layout.json` |
| glow-ring-knobs | `21:4` | required | `glow-ring-knobs.21-4.layout.json` |
| murmur-design-fx | `35:4` | required | `murmur-design-fx.35-4.layout.json` |
| murmur-desktop-play-mode | `36:4` | required | `murmur-play-view.36-4.layout.json` |
| murmur-design-engine | `37:787` | exported | `murmur-design-engine.37-787.layout.json` |
| murmur-mod-matrix | `27:265` | exported | `murmur-design-mod-matrix.27-265.layout.json` |
| murmur-basic-view | `86:4` | exported | `murmur-basic-view.86-4.layout.json` |
| master-envelope-panel | `82:4` | exported | `master-envelope-panel.82-4.layout.json` |
| murmur-preset-browser | `27:6` | exported | `murmur-preset-browser.27-6.layout.json` |
| murmur-engine-deep-editor | `28:4` | exported | `murmur-engine-deep-editor.28-4.layout.json` |
| murmur-dual-lfo-lab | `15:247` | exported | `murmur-dual-lfo-lab.15-247.layout.json` |

**Registry:** 11 frames exported, 0 pending.

---

## File locations

| Artifact | Path |
|----------|------|
| Layout specs | `plugin/src/ui/figma-connect/layouts/*.layout.json` |
| Spec manifest + tiers | `plugin/src/ui/figma-connect/layouts/manifest.json` |
| JSON schema (+ drift) | `plugin/src/ui/figma-connect/layouts/figma-layout-spec.schema.json` |
| Policy defaults | `scripts/figma_layout/policies.py` |
| Toolchain | `scripts/figma_layout.sh`, `scripts/figma_layout/` |
| C++ constants | `plugin/src/ui/PlayModeLayout.h` |
| Knob tokens | `plugin/src/ui/theme/FigmaKnobTokens.h` |
| Code Connect | `plugin/src/ui/figma-connect/*.figma.ts` |
| Human audit | `docs/FIGMA_UI_AUDIT.md` (summary — JSON is source of truth) |

---

## COMPACT reference (`4:1134`)

- Spec: [`murmur-compact-view.4-1134.layout.json`](../plugin/src/ui/figma-connect/layouts/murmur-compact-view.4-1134.layout.json)
- Frame: **320×560** (not 565)
- Header **28px** inside frame → `MurmurChromeBar` (`kChromeBarHeightCompact`)
- Body stack: scope **152** + macros **158** + output **100** + footer **30**, gaps **12**
