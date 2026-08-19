#!/usr/bin/env python3
"""Retarget Spatial preset mod routes from MasterReverb* to Quasar destinations on M3 (slot 2)."""

from __future__ import annotations

import json
import pathlib

REPO = pathlib.Path(__file__).resolve().parent.parent
SPATIAL_DIR = REPO / "content" / "presets" / "factory" / "Interstellar" / "Spatial"
QUASAR_SLOT = 2

# ModDestination ordinals — ModMatrixTypes.hpp
DST_MASTER_FX_MIX = 13
DST_MASTER_REVERB_MIX = 14
DST_MASTER_REVERB_SIZE = 15
DST_MASTER_REVERB_DECAY = 16
DST_MASTER_REVERB_PREDELAY = 17
DST_MASTER_REVERB_DIFFUSION = 18
DST_MASTER_REVERB_MOD_DEPTH = 19

DST_QUASAR_ROOM = 32
DST_QUASAR_CROSSFEED = 33
DST_QUASAR_DELAY_VOL = 34
DST_QUASAR_DELAY_TIME = 37
DST_QUASAR_DELAY_FB = 38

RETARGET = {
    DST_MASTER_REVERB_MIX: DST_MASTER_FX_MIX,
    DST_MASTER_REVERB_SIZE: DST_QUASAR_ROOM,
    DST_MASTER_REVERB_DECAY: DST_QUASAR_DELAY_FB,
    DST_MASTER_REVERB_PREDELAY: DST_QUASAR_DELAY_TIME,
    DST_MASTER_REVERB_DIFFUSION: DST_QUASAR_ROOM,
    DST_MASTER_REVERB_MOD_DEPTH: DST_QUASAR_CROSSFEED,
}


def retarget_route(route: dict) -> bool:
    if route.get("targetIndex") != QUASAR_SLOT:
        return False
    dest = route.get("destination")
    if dest not in RETARGET:
        return False
    route["destination"] = RETARGET[dest]
    return True


def main() -> None:
    updated = 0
    routes_changed = 0
    for path in sorted(SPATIAL_DIR.glob("*.pw8")):
        doc = json.loads(path.read_text(encoding="utf-8"))
        layer = doc.get("layerA", {})
        routes = layer.get("modRoutes", [])
        changed = False
        for route in routes:
            if retarget_route(route):
                routes_changed += 1
                changed = True
        if changed:
            path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
            updated += 1
    print(f"Retargeted routes in {updated} presets ({routes_changed} route rows)")


if __name__ == "__main__":
    main()
