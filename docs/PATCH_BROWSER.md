# Patch Browser

**PARTIAL** — Phases 1–3 shipped; **Phase 4a + 4c shipped** (multi-facet
`PresetMetadataFilter`, facet chip rows in overlay, prev/next wired). **Phase 4b**
(factory metadata retag) still TODO — see below.

Originally moved up from `docs/NEXT_STEPS.md` P2 to P1 (UA's plugin browser
named as the concrete UX reference). This doc is the plan of record for browsing.

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

## Shipped (phases 1–3)

1. **`PresetIndex`** (`plugin/src/content/PresetIndex.*`) — scans
   `content/presets/` recursively once, parsing just each file's
   `metadata` block (name/category/moods/tags/description/path), not the
   full patch. Exposes `filter(query, category)` and ordered `next()`/`prev()`.
   Rescans on demand (no filesystem watcher).

2. **Prev/Next on `PatchBrowserBar`** — steps through the current filtered set.

3. **`PresetBrowserOverlay`** — search + single category combo + favorites +
   scrollable list (name + category · moods subline).

4. **`FavoritesStore`** — user-side JSON outside `content/`.

### Current limitations (motivation for Phase 4)

| Capability | Today |
|---|---|
| Filter by **category** (type/role) | Yes — one combo (`ambient`, `bass`, `lead`, …) |
| Filter by **mood** | No — moods appear in list subline and free-text search only |
| Filter by **genre/context** (cinematic, score, …) | No — `genres` not indexed at all |
| Filter by **tags** | No — tags in search haystack only |
| Combine multiple metadata filters (AND) | No |
| Removable filter chips / facet UI | No |

`PatchBrowserBar::setBrowseFilters(query, category, favoritesOnly)` and
`PresetIndex::filtered()` only accept a single category string today.

---

## Phase 4: Metadata facet filters (TODO)

**Goal:** Let players narrow 278+ presets by **multiple selectable metadata
levels** — not just sonic type (`category`) but also character (**mood**),
use-case/context (**genre** — e.g. *Cinematic*, *Score*, *Sleep*), and optional
**tags** — with the same filtered set driving prev/next, the overlay list, and
(future) any compact bar summary.

### Research: schema already supports this

`patch::PatchMetadata` (`engine/include/pw8/patch/Patch.hpp`) carries four
independent dimensions — all plain strings, not DSP enums
(`docs/PATCH_FORMAT.md`):

| Dimension | Field | Cardinality | Intended meaning (master spec) |
|---|---|---|---|
| **Type / role** | `category` | one string | What the patch *is*: bass, lead, pad, ambient, seq, fx, … |
| **Character** | `moods` | string array | How it *feels*: warm, dark, bright, cinematic, evolving, … |
| **Context / use case** | `genres` | string array | Where you'd *use* it: cinematic, score, ambient-bed, worship, sleep, … |
| **Discovery** | `tags` | string array | Cross-cutting labels: reverb-showcase, fm, sub, factory, … |

**No `.pw8` schema change is required** for Phase 4 — this is indexing + UI +
(optionally) a content tagging pass.

### Research: what's in the library today (Aug 2026)

Snapshot over 278 `.pw8` files:

- **11 distinct categories** — factory folders normalize to `ambient`, `bass`,
  `lead`, `pad`, `seq`; root presets add `arp`, `fx`, `keyboard`, `pluck`, …
- **Moods** — factory bank currently stores *folder names* in `moods`
  (`basses`, `leads`, `pads`, …) for 250 presets; showcase/root presets carry
  real character tags (`cinematic` ×3, `evolving` ×12, `warm`, `dark`, …)
- **Genres** — 250× `"factory"`; root/showcase presets use meaningful values
  (`score` ×9, `ambient` ×9, `demo`, `sleep`, `sound-design`, …)
- **Tags** — mix of `factory`, engine demos (`fm`, `reverb-showcase`), and role
  hints (`bass`, `pad`, `lead`)

**Implication:** UI can ship before content is perfect (show facets that exist),
but a **factory metadata cleanup pass** (Phase 4b) will make filters like
*Cinematic* useful at scale — today only ~3 presets tag `cinematic` in `moods`.

### Proposed metadata vocabulary (canonical allow-lists for authoring)

Document in `docs/PATCH_FORMAT.md` (or a short `METADATA_VOCABULARY.md`) so
human authors and future AI generation share one language:

**Category (single, required for factory):**
`bass | lead | pad | pluck | keyboard | arp | seq | drone | chord | fx |
ambient | brass | strings | vocal-texture | texture`

**Moods (multi, character):**
`warm | dark | bright | aggressive | dreamy | metallic | glassy | organic |
digital | lush | gritty | clean | distorted | ambient | punchy | soft |
evolving | airy | cinematic | massive | desolate | spooky | restful | …`

**Genres / context (multi, use-case — where "Cinematic" lives cleanly):**
`cinematic | score | trailer | game | worship | sleep | meditation |
electronic | pop | techno | ambient-bed | sound-design | demo | factory`

**Tags (multi, freeform but prefer known tokens):**
Engine/FX demos (`fm`, `granular`, `reverb-showcase`), technique
(`mod-matrix`, `unison-demo`), provenance (`factory`, `engineering`).

*Note:* Until factory bank is retagged, **Cinematic** may appear in both
`moods` and `genres` in legacy files — Phase 4 index should match either
field when a "Context" facet is selected (union), or normalize on load.

### UX plan (UA-inspired, not pixel copy)

Replace the single **category** combo with a **facet filter row** above the
list (PLAY-mode vertical space stays tight — horizontal chip row, not a second
sidebar):

```
[ Search........................................... ]  [ Favorites ▾ ]

 Type:     [All] [Bass] [Lead] [Pad] [Ambient] [Seq] …
 Mood:     [All] [Dark] [Warm] [Evolving] [Cinematic] …
 Context:  [All] [Cinematic] [Score] [Sleep] [Sound-design] …
 Tags:     (collapsed "More…" popover — advanced, optional)

  ★ CAVERN ECHO          ambient · evolving
    DUNE SIETCH DRONE    ambient · dark, cinematic
    …
```

Interaction rules:

- **AND across dimensions** — Type=Bass AND Mood=Dark AND Context=Cinematic
- **OR within a dimension** — Mood=Dark OR Warm (shift-click or multi-select
  chips; v1 can stay single-select per row for simplicity)
- **"All" clears that row** — same as today's "All categories"
- **Active filters persist** — closed overlay → prev/next + bar still respect
  facets; store in memory (v1) or Application Support prefs (v2)
- **Empty facet values hidden** — only show chips with ≥1 matching preset after
  other filters applied (faceted search — selecting Bass hides Mood chips with
  zero bass matches)
- **List subline** — show `category · moods` today; add `genres` when non-empty

Compact bar (optional polish): truncated filter summary next to patch name
e.g. `Bass · Dark · Cinematic`.

### Architecture plan

**4a — `PresetIndex` metadata index**

```cpp
struct PresetMetadataFilter {
    juce::String query;           // existing free-text
    juce::String category;        // empty = all
    juce::String mood;            // matches if any moods[] equals (case-ins)
    juce::String genre;           // matches if any genres[] equals
    juce::String tag;             // matches if any tags[] equals
    bool favoritesOnly = false;
};

struct PresetEntry {
    // … existing …
    juce::StringArray genres;     // NEW — parse from metadata.genres
};

// NEW helpers
juce::StringArray uniqueMoods(const PresetMetadataFilter& context) const;
juce::StringArray uniqueGenres(const PresetMetadataFilter& context) const;
juce::StringArray uniqueTags(const PresetMetadataFilter& context) const;
juce::Array<PresetEntry> filtered(const PresetMetadataFilter&) const;
```

`unique*()` with `context` enables **faceted counts** — each helper runs
`filtered(contextWithThatRowCleared)` and tallies values still available.

Extend `PatchBrowserBar::setBrowseFilters` → `setBrowseFilters(PresetMetadataFilter)`.

**4b — Factory metadata cleanup (content, parallel track)**

Script or one-off tool: for each factory `.pw8`, set:

- `category` ← folder name (already mostly true)
- `moods` ← procedurally chosen from character vocabulary (replace folder name)
- `genres` ← `["electronic"]` or contextual assignment table
- `tags` ← `["factory", "<category>"]`

Showcase presets (`dune-*`, `bloom-showcase`, …): promote `cinematic` from mood
→ `genres` where appropriate; keep mood for character (`dark`, `massive`).

**4c — `PresetBrowserOverlay` UI**

- New `MetadataFacetRow` component — scrollable chip strip per dimension
- Wire `onChange` → `rebuildList()` + refresh other rows' available chips
- Replace `browseCategory()` with `browseFilter()` returning full struct
- Tests: `PresetIndex` unit tests for AND semantics, case insensitivity,
  faceted `uniqueMoods()` under partial filter

**4d — MCP / Patchwork alignment**

`mcp_server/content.py` `list_presets()` already returns metadata — extend with
optional `mood`/`genre`/`tag` query params so agent browsing matches plugin UX.

### Sequencing

| Step | Effort | User-visible |
|---|---|---|
| 4a Index + filter struct | Small | None (API only) |
| 4c Facet chip UI | Medium | **Yes** — multi-level filters work |
| 4b Factory retag | Medium | **Yes** — Cinematic/Score filters populate |
| 4d MCP parity | Small | Agent tooling |
| Persist filters to prefs | Small | Convenience |

Ship **4a + 4c** first with existing metadata (sparse but honest); **4b**
unlocks the full vocabulary at scale.

### Explicitly still out of scope

- New top-level metadata fields (`contexts[]`, `energy`, BPM range) — defer
  unless genre overload proves insufficient after retag
- User-editable metadata from plugin (preset *manager*, not browser)
- Full-text search engine / fuzzy rank — plain substring haystack is enough for
  v1 (already implemented)

## Explicitly out of scope for this pass

- Save/rename/delete/organize-into-user-folders -- a real preset *manager*,
  not just a browser. Worth its own pass once browsing itself ships and
  proves out the interaction patterns.
- Audio preview/audition without loading (would need a preview voice
  independent of the currently-loaded patch) -- real scope, deliberately
  deferred rather than silently assumed.
- Any change to `.pw8`'s schema -- everything here reads fields that
  already exist.
