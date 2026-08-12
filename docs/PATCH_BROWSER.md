# Patch Browser

**PLANNED, now prioritized** (moved up from `docs/NEXT_STEPS.md` P2 #2 to P1
by explicit direction -- UA's plugin browser named as the concrete UX
reference). This doc exists so the reprioritization has a real plan behind
it, not just a moved bullet point.

## Why now, specifically

`PatchBrowserBar`'s own header comment has said this since UI GATE 1:
*"prev/next/save preset BROWSING needs a content-scanning system that
doesn't exist yet ... PLANNED: real prev/next once a preset index exists."*
That was a minor, deferrable gap when `content/presets/` held 14 engineering
test patches. It no longer is: the repo now ships **278 presets** (28
root-level + the 250-patch factory bank across Basses/Leads/Pads/Sequences/
Ambient, `docs/ROADMAP.md` Phase 19), all reachable today only by clicking
"Load...", getting a native OS file dialog, and manually navigating into
`content/presets/factory/<Category>/` by hand. That's the single biggest
"this feels unfinished" moment for anyone actually playing the synth, worse
now than when this was first flagged, not better.

## What UA's browser gets right (the reference, not a pixel copy)

Universal Audio's plugin preset browser is the named target because it gets
a few things right that generalize well beyond their specific UI chrome:

- A persistent **search field** -- type to filter by name, always visible,
  not buried in a menu.
- A **category/type filter** down one side -- narrow to "Bass" or "Ambient"
  before scanning names.
- **Prev/Next stepping** through the *current filtered result set* without
  reopening the browser -- once you've found roughly the right neighborhood,
  arrow through it. This is the same interaction language the (also
  currently unmerged) `wavetable-3d-mesh` branch's `WavetableStackView`
  already added `"<"`/`">"` browse arrows for -- reusing it here keeps the
  whole plugin speaking one consistent UX vocabulary instead of two.
- **Favorites/starring** -- a fast personal shortlist independent of the
  folder structure.
- Preset metadata visible inline (category, sometimes author) without
  opening the file.

## What this doesn't need to invent

`patch::PatchMetadata` already has `category`, `moods`, `tags`, and
`description` (`docs/PATCH_FORMAT.md`) -- every field a browser would filter
or search on already exists in every `.pw8` file shipped so far, factory
bank included. This is a UI + indexing problem, not a schema problem.

It's also already proven fast at this exact scale: `mcp_server/content.py`'s
`list_presets()` recursively parses all 278 `.pw8` files' metadata on every
call with no caching at all, and it's imperceptibly fast. A C++ equivalent
scanning `content/presets/` once at plugin load (not per keystroke) has
enormous headroom.

## Architecture

1. **`PresetIndex`** (new, pure data/logic, no UI dependency) -- scans
   `content/presets/` recursively once, parsing just each file's
   `metadata` block (name/category/moods/tags/description/path), not the
   full patch -- cheap even at hundreds of entries, and doesn't need
   `PatchSerializer`'s full `fromJson` machinery for fields the browser
   doesn't use. Exposes `filter(query, category)` and ordered `next()`/
   `prev()` relative to whatever's currently loaded. Rescans on demand
   (a manual refresh, not a filesystem watcher -- content changes by
   rebuilding/reinstalling, not live editing, so a watcher would be
   solving a problem that doesn't exist yet).

2. **Prev/Next arrows in the existing `PatchBrowserBar` strip** -- the
   smallest, highest-leverage slice, and literally the exact thing that
   component's own comment has been waiting on. Ship this alone before the
   full browser panel; it's useful immediately and de-risks `PresetIndex`
   in production before more UI is built on top of it.

3. **A real browser panel** (second phase) -- search field + category
   filter + a scrollable list (name + category chip + moods), click to
   load. Likely an overlay/expandable panel off `PatchBrowserBar` (click the
   patch name, or a new dedicated button) rather than a permanently-docked
   pane -- PLAY mode's vertical space is already tight (the unmerged
   wavetable mesh work already grew the window once for the same reason).

4. **Favorites** (third phase) -- needs a small persisted user-state file
   *outside* `content/` (e.g. `~/Library/Application Support/Patchwork
   Eight/favorites.json`, matching the installer's own Application Support
   convention from `scripts/package_macos.sh`) -- favoriting a factory
   preset can't mean writing into the installed/shipped files themselves.

## Explicitly out of scope for this pass

- Save/rename/delete/organize-into-user-folders -- a real preset *manager*,
  not just a browser. Worth its own pass once browsing itself ships and
  proves out the interaction patterns.
- Audio preview/audition without loading (would need a preview voice
  independent of the currently-loaded patch) -- real scope, deliberately
  deferred rather than silently assumed.
- Any change to `.pw8`'s schema -- everything here reads fields that
  already exist.
