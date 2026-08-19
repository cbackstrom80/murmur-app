#!/usr/bin/env python3
"""Create MASTER SPATIAL factory bank from embedded Interstellar Spatial presets."""

from __future__ import annotations

import json
import pathlib
import shutil

REPO = pathlib.Path(__file__).resolve().parent.parent
SPATIAL = REPO / "content" / "presets" / "factory" / "Interstellar" / "Spatial"
OUT = REPO / "content" / "presets" / "factory" / "MasterSpatial"
COUNT = 20


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    sources = sorted(SPATIAL.glob("*.pw8"))[:COUNT]
    for i, src in enumerate(sources, start=1):
        doc = json.loads(src.read_text(encoding="utf-8"))
        meta = doc.setdefault("metadata", {})
        base_name = meta.get("name", src.stem)
        meta["name"] = f"MASTER SPATIAL {base_name}"
        meta["category"] = "master-spatial"
        tags = meta.setdefault("tags", [])
        if "master-spatial" not in tags:
            tags.append("master-spatial")
        meta["description"] = (
            f"In-MURMUR master spatial showcase #{i:02d} — inline Quasar on M3. "
            + meta.get("description", "")[:120]
        )
        out_name = f"{i:03d}-{src.stem.split('-', 1)[-1]}.pw8"
        (OUT / out_name).write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
    readme = OUT / "README.md"
    readme.write_text(
        "# Master Spatial factory bank\n\n"
        f"{COUNT} headphone-first pads with embedded QUASAR binaural on master M3.\n"
        "Derived from Interstellar/Spatial embedded presets.\n",
        encoding="utf-8",
    )
    print(f"Wrote {len(sources)} presets to {OUT}")


if __name__ == "__main__":
    main()
