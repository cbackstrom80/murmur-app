# Preview MIDI Library

261 real, categorized MIDI phrase files, vendored from
`patchwork-pipeline/data/midi_library/` (provenance confirmed by the
project owner there — see that repo's own README) for use as the real
render source material for MURMUR's patch preview MP3 pipeline
(`murmur-web/scripts/render-patch-previews.ts`).

```
Arps/       66 files
Pads/       41 files
Leads/      38 files
Sequences/  35 files
Bass/       81 files
```

Only these 5 categories were copied — `patchwork-pipeline`'s corpus also
has `Chords/` and `Strings/`, not needed for this pipeline.

Filenames encode BPM (e.g. `DigitalArps-01-75-D.mid` = pattern
"DigitalArps", index 01, 75 BPM, key D) but carry **no embedded
`set_tempo` MIDI meta event** — a renderer must pass `--bpm` explicitly
from the filename, or clips silently render at murmur-render's 120bpm
default.
