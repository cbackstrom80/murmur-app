# Classic Synth Preset Naming Guide

Actionable reference for MURMUR factory content: what vintage archetypes to capture, how to name them safely, and how tags map to the browser. Implementation lives in `scripts/generate_factory_presets.py`.

**Rule:** Display names and primary tags must **never** use trademarked hardware names (Juno, Prophet, DX7, Minimoog, Jupiter, OB-X, CS-80, 303, etc.). Use synthesis vocabulary and era/mood tags instead.

---

## 1. Taxonomy vs PW8 folders

Serious synth banks ship 8–15 top-level categories. PW8 keeps **five browser folders** (50 presets each) and uses **metadata tags** for everything else.

| PW8 folder | Classic roles covered | Gaps filled via tags |
|---|---|---|
| **Basses** | Sub, mono growl, pulse, wobble | `ladder-bass`, `acid-line`, `rubber-funk`, `fm-sub` |
| **Leads** | Sync solo, portamento, scream | `sync-lead`, `brass-stab`, `formant-vox`, `chip-lead` |
| **Pads** | Lush sustain, wide stereo | `poly-sweet`, `string-ensemble`, `dual-layer`, `wt-evolve` |
| **Sequences** | Arp, gated patterns, pluck-seq | `acid-seq`, `gated-80s`, `arp-bell`, `stab-seq` |
| **Ambient** | Drone, wash, soundscape | `hymn-swell`, `gran-cloud`, `fx-riser`, `resonant-cave` |

**Tag-only roles** (no extra folder): Keys (`organ-keys`, `pluck`, `keys`), Brass (`brass-stab`), Strings (`string-ensemble`), FX (`fx-riser`, `fx-hit`), Bell (`fm-glass`, `arp-bell`).

### Tag layers (browser filtering)

1. **Role** — matches folder: `bass`, `lead`, `pad`, `sequence`, `ambient` (plus `factory`, category key)
2. **Archetype** — sonic family: `ladder-bass`, `poly-sweet`, `sync-lead`, `fm-glass`, `wt-scan`, …
3. **Era/mood** (optional): `70s-analog`, `80s-pop`, `90s-digital`, `cinematic`

---

## 2. Sonic archetypes (safe naming cues)

Technical fingerprints for content authors. **Left column = tag; right = recipe hint.** No brand names in titles.

| Archetype tag | Sonic fingerprint | Safe name cues |
|---|---|---|
| `ladder-bass` | Saw/square, 24 dB LP, fast filter env, saturation | Ladder, Coil, Crawl, Undertow |
| `acid-line` | High-res LP sweep, accent via velocity→cutoff | Squelch, Rubber, Accent, Slide |
| `rubber-funk` | Saw + sub sine, medium LP, portamento | Rubber, Funk, Pulse, Coil |
| `fm-sub` | FM low carrier, velocity→index, sine sub layer | Ratio, Metallic, Sub, Pulse |
| `sync-lead` | Hard sync waveform, fast filter bite | Sync, Tear, Rip, Voltage |
| `fifth-lead` | Dual saw detune, 5th stack | Fifth, Stack, Beam, Interval |
| `brass-stab` | Dual saw, filter env pluck, chorus | Bronze, Stab, Section, Brass |
| `formant-vox` | WT formant morph + granular dust | Formant, Vox, Vocal, Facet |
| `chip-lead` | Hard square, phase grit, fast motion | Pixel, Chip, Arcade, Static |
| `poly-sweet` | Single saw + chorus, HPF, light detune | Chorus, Haze, Fifth, Bucket |
| `string-ensemble` | 3× saw unison, chorus, HPF | Ensemble, Velvet, Stack, Section |
| `dual-layer` | Layer A/B morph, swell envelopes | Dual, Swell, Layer, Hymn |
| `wt-scan` | WT position mod, digital→analog LP | Scan, Table, Gargle, Facet |
| `wt-evolve` | Slow LFO→WT pos, long release | Drift, Morph, Ripple, Wave |
| `fm-glass` | FM bell ratio, short decay, chorus | Glass, Tine, Bell, Clang |
| `gated-80s` | Square + arp, tight amp env, comp | Gate, Motor, Neon, Pulse |
| `acid-seq` | Resonant bass seq, accent envelope | Squelch, Ratchet, Line, Step |
| `arp-bell` | FM/pluck + arp Up, short decay | Chime, Tine, Glass, Motif |
| `hymn-swell` | Long attack, delayed vibrato, chorus | Hymn, Swell, Brilliance, Dawn |
| `gran-cloud` | Granular texture + resonator body | Cloud, Spray, Dust, Shimmer |
| `fx-riser` | Noise sweep + pitch rise + reverb | Photon, Riser, Surge, Void |
| `resonant-cave` | Modal resonator, huge space | Cavern, Hollow, Depth, Field |
| `organ-keys` | Additive drawbar harmonics | Drawbar, Cathedral, Rotary, Chord |
| `drawbar` | Fixed harmonic stack, leslie-ish motion | Drawbar, Pipe, Rotary |

Target **~30–40%** of the 250 bank toward recognizable vintage archetypes; keep the rest in PW8's geological/atmospheric voice.

---

## 3. Naming playbook

### Display name formula

**Two-word Title Case:** `[Texture/Material/Synthesis] + [Role/Motion/Color]`

Examples: `Bronze Stab`, `Fifth Stack`, `Glass Tine`, `Table Gargle`, `Shale Field` (existing geological voice).

File: `{nn}-{kebab-case}.pw8` — e.g. `51-bronze-stab.pw8`.

### Vocabulary pools

| Pool | Purpose | Examples |
|---|---|---|
| Geological (existing) | PW8 identity | Shale, Glacier, Nebula, Ochre, Fen |
| Synthesis adjectives | Classic evocation | Bronze, Squelch, Sync, Glass, Drawbar, Formant, Fifth, Ladder |
| Synthesis nouns | Role hint | Stab, Stack, Tine, Scan, Swell, Section, Line, Chime |

Generator blends **~30%** synthesis-evocative names with **~70%** geological/atmospheric (deterministic per seed).

### Description template

```
Factory {Category} patch #{nn}. Engines: {list}. Procedurally generated (seed {n}).
Archetype: {human-readable archetype phrase}.
```

Archetype phrase comes from assigned tag(s), e.g. `poly-sweet chorus pad`, `ladder-filter acid bass`.

### Industry patterns (reference)

- **Arturia / u-he:** Category prefix or tag, generic descriptor (`BA Classic Saw Bass`, `#Bass #Cinematic`)
- **Sequential / Oberheim factory lists:** Poetic + technical hybrid (`Bronze Pad`, `Syncbomb`) — inspiration only, do not clone names at scale
- **PW8:** Geological display names + archetype tags carry the "vintage synth people will get it" signal

---

## 4. Reference preset concepts (18)

Safe names for manual or scripted expansion. Engines: Classic, Wavetable, FmPm, Additive, PhaseShape, Granular, NoiseChaos, Resonator.

| Name | Folder | Recipe hint | Tags |
|---|---|---|---|
| BRONZE STAB | Leads | Classic ×2 saw detune + filter pluck + chorus | `brass-stab`, `poly-sweet` |
| FIFTH STACK | Pads | Saw @1.0 + @1.5, chorus, slow attack | `poly-sweet`, `fifth-stack` |
| LADDER SQUELCH | Basses | Square, high LP res, velocity→cutoff | `ladder-bass`, `acid-line` |
| GLASS TINE | Sequences | FmPm bell + arp 1/8 | `fm-glass`, `arp-bell`, `keys` |
| SYNC TEAR | Leads | Classic sync, filter bite | `sync-lead`, `70s-analog` |
| TABLE GARGLE | Pads | WT scan + LP, long release | `wt-scan`, `wt-evolve` |
| DUAL SWELL | Pads | Layer morph, long envelopes | `dual-layer`, `hymn-swell` |
| STRING ENSEMBLE | Pads | 3× saw unison + chorus | `string-ensemble`, `poly-sweet` |
| RUBBER FUNK | Basses | Saw + sub, portamento, saturation | `rubber-funk`, `ladder-bass` |
| FM SUB PULSE | Basses | FmPm + Classic sine sub | `fm-sub`, `mono-growl` |
| DRAWBAR CHORD | Pads | Additive harmonics + PhaseShape pan | `organ-keys`, `drawbar` |
| CLAV STAB | Sequences | WT pluck, fast filter env | `keys`, `pluck` |
| GATED MOTOR | Sequences | Square arp, tight gate feel | `gated-80s`, `acid-seq` |
| VOX FORMANT | Leads | WT formant + Granular dust | `formant-vox` |
| COP SHOW STAB | Leads | Dual saw brass, short decay | `brass-stab`, `70s-analog`, `cinematic` |
| RESONANT CAVE | Ambient | Resonator + Granular, huge reverb | `resonant-cave`, `gran-cloud` |
| PHOTON RISER | Ambient | NoiseChaos sweep + Additive rise | `fx-riser`, `cinematic` |
| PIXEL SCREAM | Leads | Square + PhaseShape grit | `chip-lead`, `90s-digital` |

---

## 5. Trademark pitfalls — what NOT to do

| Avoid | Safer alternative |
|---|---|
| Hardware names in titles: Juno, Prophet, DX7, Minimoog, Jupiter, 303 | `Poly Sweet Pad`, `Sync Fifth Lead`, `Glass FM Bell`, `Ladder Bass` |
| Artist/song titles: `Take On Me Lead`, `Blade Runner Pad` | `Bright Square Lead`, `Cinematic Noir Pad` |
| "Official" / "Authentic [Brand]" marketing | "Vintage-inspired analog poly" |
| Model numbers as titles: `OB-84`, `Prophet-5 Preset #11` | `Bronze Section`, `Classic Fifth Brass` |
| Exact factory patch name clones at scale | Original names; similar sound, different title |
| Cliché fatigue: `Warm Analog Pad`, `Fat Bass`, `Supersaw Lead 01` | Evocative two-word compounds |

**OK in descriptions (nominative use):** "FM bell in the spirit of 1983 digital polysynth factory programs" — factual, not in the product title.

---

## 6. Implementation

`scripts/generate_factory_presets.py`:

- `ARCHETYPE_TAGS` — per-category tag pool; 1–2 tags added deterministically per preset
- `SYNTH_ADJECTIVES` / `SYNTH_NOUNS` — classic-evocative name words (~30% blend in `make_name()`)
- Description suffix: `Archetype: {phrase}.`
- Macro description: `Performance macro.` (not "Reserved")

After editing the script:

```bash
python3 scripts/generate_factory_presets.py
python3 scripts/validate_content_refs.py
```

Spot-check: `schemaVersion: 2`, `tags` includes archetype strings, description contains `Archetype:`.

---

## Related docs

- `docs/PATCH_FORMAT.md` — schema v2, metadata fields
- `docs/COMPETITIVE_ANALYSIS.md` — feature parity context
- `content/presets/factory/MANIFEST.json` — generated bank index
