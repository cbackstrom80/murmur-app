# MURMUR — Presets

MURMUR ships with **800 factory presets** — a complete starting library across five musical roles. Every preset includes six Knobs of Interest, standard MIDI performance routes, and patch-specific macro names.

---

## Factory bank structure

| Category | Count | Folder | Typical use |
|----------|-------|--------|-------------|
| **Basses** | 160 | `Presets/factory/Basses/` | Sub, FM, acid, hoover, rubber funk |
| **Leads** | 160 | `Presets/factory/Leads/` | Sync, brass, formant, portamento solos |
| **Pads** | 160 | `Presets/factory/Pads/` | Strings, washes, dream-pop, cinematic |
| **Sequences** | 160 | `Presets/factory/Sequences/` | Arps, plucks, gated 80s, acid lines |
| **Ambient** | 160 | `Presets/factory/Ambient/` | Drones, gran clouds, hymn swells, FX beds |

Installed location:

```
~/Library/Application Support/MURMUR/Presets/factory/
```

---

## Naming convention

Factory files use numbered slots plus descriptive names:

```
061-glass-hymn.pw8
08-velvet-chord.pw8
097-oracle-dawn.pw8
```

- **01–50** — Original procedural factory batch
- **51–60** — Mew / M83-inspired showcase set
- **061–160** — Genre expansion (hoover basses, dream-pop, house, cinematic)

---

## Metadata & search

Each `.pw8` preset stores searchable metadata:

| Field | Example | Browse use |
|-------|---------|------------|
| **name** | Glass Hymn | Display + text search |
| **category** | pad | Category filter |
| **moods** | dreamy, wide | Subline + search |
| **tags** | dream-pop, shimmer, factory | Tag search |
| **description** | Archetype + character notes | List subline |

In **BROWSE**, type any keyword — search matches name, description, category, moods, and tags.

---

## Category guide

### Basses

Short envelopes, mono-ish character, ladder and FM archetypes. Look for tags: `hoover-bass`, `acid-line`, `rubber-funk`, `fm-sub`.

**Try:** mod wheel for filter sweep; Macro 1 often maps to PUNCH or DRIVE.

### Leads

Cutting sync leads, brass stabs, formant vocals. CC74 (brightness) wired on many leads.

**Try:** velocity → filter; expression for bite.

### Pads

Slow attacks, long releases, chorus and reverb. Ben MVP includes **legato-friendly polyphony** (16–24 voice slots) so chord changes don't stack infinitely.

**Try:** aftertouch on pads; Macro 6 often controls SPACE / reverb depth.

### Sequences

Plucks, bells, resonators, arpeggiated motion. Velocity may map to operator level.

**Try:** enable arpeggiator on patches that ship with arp enabled; Macro 1 often RATE or GATE.

### Ambient

Granular clouds, noise drones, evolving wavetable washes. Aftertouch and long FX tails.

**Try:** Macro 6 SPACE; low mod wheel for darkness.

---

## Showcase presets

Additional demos live at:

```
~/Library/Application Support/MURMUR/Presets/showcase/
```

Root-level engineering demos remain in the repo under `content/presets/` for developers.

---

## Favorites

1. Open **BROWSE**.
2. Click the **star** on a preset.
3. Filter to favorites in the overlay.

Stored at: `~/Library/Application Support/MURMUR/favorites.json` (survives preset reinstalls).

---

## Saving your own patches

1. Tweak macros, filter, or mod routes.
2. Click **SAVE** in the patch bar.
3. Choose a location — your user preset folder or project folder.

Format: **`.pw8`** (JSON, schema version 2). Portable across MURMUR installs.

---

## Preset standards (factory)

Every factory preset includes:

- **≥4 Knobs of Interest** (target 6)
- **Mod wheel → filter cutoff** (category-tuned depth)
- **Expression (CC11)** route
- **Velocity** route
- **Primary macro** wired in mod matrix
- **Aftertouch** on pads and ambient
- **CC74 brightness** route

Pads additionally: **amp legato enabled**, polyphony scaled for unison.

---

## Regenerating factory content (developers)

```bash
python3 scripts/generate_factory_presets.py
python3 scripts/generate_factory_presets.py --fix-all   # normalize MIDI + pad playability
```

→ [PLAY Mode](PLAY_MODE.md) · [Performance](PERFORMANCE.md)
