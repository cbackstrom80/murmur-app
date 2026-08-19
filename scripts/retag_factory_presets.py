#!/usr/bin/env python3
"""Retag factory-bank .pw8 metadata for preset-browser facet filters (PATCH_BROWSER Phase 4b).

Replaces folder-name moods (e.g. "pads", "basses") and bare genres: ["factory"] with
character moods, use-case genres, and cleaned tags derived from archetype tags,
category, and name/description tokens.

Targets the original 250-patch bank (pw8-factory-{category}-{seed}) plus warp/template
demos. Skips presets that already carry rich metadata (genre expansion, Interstellar, …).

Run from repo root:
    python3 scripts/retag_factory_presets.py
    python3 scripts/retag_factory_presets.py --dry-run
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
FACTORY_ROOT = REPO_ROOT / "content" / "presets" / "factory"

FOLDER_MOOD_PLACEHOLDERS = frozenset(
    {
        "basses",
        "leads",
        "pads",
        "sequences",
        "ambient",
        "bass",
        "lead",
        "pad",
        "seq",
        "sequence",
        "sidechain",
        "warp",
        "interstellar",
    }
)

ARCHETYPE_MOODS: dict[str, list[str]] = {
    "ladder-bass": ["dark", "punchy"],
    "rubber-funk": ["warm", "punchy"],
    "fm-sub": ["dark", "massive"],
    "acid-line": ["aggressive", "bright"],
    "sync-low": ["bright", "aggressive"],
    "mono-growl": ["dark", "gritty"],
    "sync-lead": ["bright", "aggressive"],
    "fifth-lead": ["bright", "lush"],
    "brass-stab": ["punchy", "bright"],
    "formant-vox": ["organic", "digital"],
    "portamento": ["warm", "expressive"],
    "chip-lead": ["digital", "bright"],
    "poly-sweet": ["warm", "lush"],
    "dual-layer": ["lush", "evolving"],
    "string-ensemble": ["warm", "lush"],
    "brass-pad": ["warm", "cinematic"],
    "wt-evolve": ["evolving", "airy"],
    "formant-choir": ["dreamy", "lush"],
    "acid-seq": ["aggressive", "bright"],
    "gated-80s": ["punchy", "bright"],
    "arp-bell": ["bright", "glassy"],
    "sync-seq": ["bright", "punchy"],
    "fm-pluck": ["bright", "punchy"],
    "stab-seq": ["punchy", "aggressive"],
    "hymn-swell": ["warm", "restful"],
    "wt-wash": ["airy", "evolving"],
    "noise-drone": ["dark", "desolate"],
    "fx-riser": ["evolving", "massive"],
    "gran-cloud": ["dreamy", "airy"],
    "resonant-cave": ["dark", "spooky"],
}

ARCHETYPE_GENRES: dict[str, list[str]] = {
    "ladder-bass": ["electronic", "techno"],
    "rubber-funk": ["electronic", "pop"],
    "fm-sub": ["electronic", "sound-design"],
    "acid-line": ["electronic", "techno"],
    "sync-low": ["electronic", "pop"],
    "mono-growl": ["electronic", "sound-design"],
    "sync-lead": ["electronic", "pop"],
    "fifth-lead": ["electronic", "pop"],
    "brass-stab": ["cinematic", "score"],
    "formant-vox": ["electronic", "pop"],
    "portamento": ["electronic", "pop"],
    "chip-lead": ["electronic", "game"],
    "poly-sweet": ["ambient-bed", "electronic"],
    "dual-layer": ["ambient-bed", "cinematic"],
    "string-ensemble": ["cinematic", "score"],
    "brass-pad": ["cinematic", "score"],
    "wt-evolve": ["ambient-bed", "electronic"],
    "formant-choir": ["cinematic", "score"],
    "acid-seq": ["electronic", "techno"],
    "gated-80s": ["electronic", "pop"],
    "arp-bell": ["electronic", "pop"],
    "sync-seq": ["electronic", "techno"],
    "fm-pluck": ["electronic", "pop"],
    "stab-seq": ["electronic", "techno"],
    "hymn-swell": ["ambient-bed", "meditation"],
    "wt-wash": ["ambient-bed", "meditation"],
    "noise-drone": ["ambient-bed", "sound-design"],
    "fx-riser": ["cinematic", "trailer"],
    "gran-cloud": ["ambient-bed", "sound-design"],
    "resonant-cave": ["ambient-bed", "cinematic"],
}

CATEGORY_MOODS: dict[str, list[str]] = {
    "bass": ["punchy", "dark"],
    "lead": ["bright", "aggressive"],
    "pad": ["dreamy", "lush"],
    "seq": ["punchy", "evolving"],
    "ambient": ["airy", "evolving"],
    "warp": ["digital", "evolving"],
    "sidechain": ["electronic", "vocal"],
    "interstellar": ["cinematic", "cosmic"],
    "templates": ["metallic", "glassy"],
}

CATEGORY_GENRES: dict[str, list[str]] = {
    "bass": ["electronic"],
    "lead": ["electronic", "pop"],
    "pad": ["ambient-bed", "electronic"],
    "seq": ["electronic", "techno"],
    "ambient": ["ambient-bed", "meditation"],
    "warp": ["demo", "sound-design"],
    "sidechain": ["electronic", "pop"],
    "interstellar": ["cinematic", "score"],
    "templates": ["demo", "sound-design"],
}

TOKEN_MOODS: list[tuple[tuple[str, ...], list[str]]] = [
    (("dusk", "mist", "haze", "gentle", "velvet", "wistful", "pale", "feather", "opal"), ["dreamy", "soft"]),
    (("neon", "chrome", "solar", "voltage", "pixel", "signal", "wire"), ["bright", "digital"]),
    (("void", "abyss", "cavern", "wraith", "sludge", "iron", "basalt", "trench", "fathom"), ["dark", "massive"]),
    (("hymn", "swell", "aurora", "halcyon", "amber", "dawn"), ["warm", "lush"]),
    (("squelch", "acid", "grind", "razor", "spike", "fang", "wobble"), ["aggressive", "gritty"]),
    (("drift", "bloom", "expanse", "field", "wash", "cloud"), ["airy", "evolving"]),
    (("glass", "prism", "crystal", "tine", "chime"), ["glassy", "bright"]),
    (("rumble", "growl", "sub", "thud", "anchor"), ["dark", "punchy"]),
    (("motor", "ratchet", "clock", "pulse", "loop", "step"), ["punchy", "evolving"]),
    (("score", "epic", "trailer", "titan", "vast", "nova", "horizon", "apex"), ["cinematic", "massive"]),
    (("club", "garage", "house", "floor", "night"), ["punchy", "bright"]),
    (("shimmer", "cascade", "dream", "comfort"), ["dreamy", "lush"]),
    (("formant", "vowel", "vocal", "talk"), ["organic", "digital"]),
    (("warp", "bend", "morph"), ["digital", "evolving"]),
    (("feedback", "bell", "metallic"), ["metallic", "glassy"]),
]

TOKEN_GENRES: list[tuple[tuple[str, ...], list[str]]] = [
    (("score", "epic", "trailer", "titan", "vast", "horizon", "apex", "brass", "ensemble"), ["cinematic", "score"]),
    (("club", "garage", "house", "motor", "floor", "rave", "hoover"), ["electronic", "techno"]),
    (("neon", "dream", "shimmer", "velvet", "pop", "80s"), ["pop", "electronic"]),
    (("hymn", "meditation", "sleep", "drone", "wash"), ["ambient-bed", "meditation"]),
    (("acid", "techno", "seq", "motor"), ["electronic", "techno"]),
    (("demo", "showcase", "warp", "engineering", "template", "feedback"), ["demo", "sound-design"]),
    (("vocoder", "sidechain", "vocal"), ["electronic", "pop"]),
]


def _normalize_category(category: str) -> str:
    cat = category.strip().lower()
    if cat in ("sequences", "sequence"):
        return "seq"
    if cat.endswith("s") and cat[:-1] in CATEGORY_MOODS:
        return cat[:-1]
    return cat


def _needs_retag(meta: dict) -> bool:
    moods = [m.strip().lower() for m in meta.get("moods") or [] if m.strip()]
    genres = [g.strip().lower() for g in meta.get("genres") or [] if g.strip()]
    category = _normalize_category(meta.get("category") or "")

    mood_bad = not moods or all(m in FOLDER_MOOD_PLACEHOLDERS or m == category for m in moods)
    genre_bad = genres == ["factory"] or (len(genres) == 1 and genres[0] == "factory")
    return mood_bad or genre_bad


def _unique_ordered(values: list[str], limit: int = 6) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for value in values:
        key = value.strip().lower()
        if not key or key in seen:
            continue
        seen.add(key)
        out.append(key)
        if len(out) >= limit:
            break
    return out


def _scan_tokens(text: str, table: list[tuple[tuple[str, ...], list[str]]]) -> list[str]:
    hay = text.lower()
    found: list[str] = []
    for tokens, labels in table:
        if any(token in hay for token in tokens):
            found.extend(labels)
    return found


def retag_metadata(meta: dict) -> dict | None:
    if not _needs_retag(meta):
        return None

    category = _normalize_category(meta.get("category") or "")
    tags = [t.strip().lower() for t in meta.get("tags") or [] if t.strip()]
    name = (meta.get("name") or "").strip()
    description = (meta.get("description") or "").strip()
    scan_text = f"{name} {description}"

    moods: list[str] = []
    genres: list[str] = []

    for tag in tags:
        moods.extend(ARCHETYPE_MOODS.get(tag, []))
        genres.extend(ARCHETYPE_GENRES.get(tag, []))

    moods.extend(CATEGORY_MOODS.get(category, []))
    genres.extend(CATEGORY_GENRES.get(category, []))
    moods.extend(_scan_tokens(scan_text, TOKEN_MOODS))
    genres.extend(_scan_tokens(scan_text, TOKEN_GENRES))

    existing_moods = [
        m.strip().lower()
        for m in meta.get("moods") or []
        if m.strip() and m.strip().lower() not in FOLDER_MOOD_PLACEHOLDERS
    ]
    moods = _unique_ordered(existing_moods + moods, limit=5)
    if not moods:
        moods = _unique_ordered(CATEGORY_MOODS.get(category, ["evolving"]), limit=3)

    genres = _unique_ordered(genres + ["factory"], limit=5)
    if genres == ["factory"]:
        genres = _unique_ordered(CATEGORY_GENRES.get(category, ["electronic"]) + ["factory"], limit=5)

    cleaned_tags = _unique_ordered(
        tags + [category] + ["factory"] + moods[:2],
        limit=10,
    )

    updated = dict(meta)
    updated["moods"] = moods
    updated["genres"] = genres
    updated["tags"] = cleaned_tags
    return updated


def retag_file(path: pathlib.Path, dry_run: bool) -> bool:
    data = json.loads(path.read_text(encoding="utf-8"))
    meta = data.get("metadata")
    if not isinstance(meta, dict):
        return False

    updated = retag_metadata(meta)
    if updated is None:
        return False

    if dry_run:
        return True

    data["metadata"] = updated
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Retag factory preset metadata for browser facets.")
    parser.add_argument("--dry-run", action="store_true", help="Report counts without writing files.")
    args = parser.parse_args()

    changed = 0
    skipped = 0
    samples: list[tuple[str, list[str], list[str]]] = []

    for path in sorted(FACTORY_ROOT.rglob("*.pw8")) + sorted(FACTORY_ROOT.rglob("*.murmur")):
        data = json.loads(path.read_text(encoding="utf-8"))
        meta = data.get("metadata") or {}
        if not _needs_retag(meta):
            skipped += 1
            continue
        if retag_file(path, args.dry_run):
            changed += 1
            if len(samples) < 6:
                updated = retag_metadata(meta)
                assert updated is not None
                samples.append(
                    (
                        str(path.relative_to(FACTORY_ROOT)),
                        updated["moods"],
                        updated["genres"],
                    )
                )

    mode = "would retag" if args.dry_run else "retagged"
    print(f"{mode} {changed} factory presets ({skipped} already rich / skipped).")
    if samples:
        print("\nSample outputs:")
        for rel, moods, genres in samples:
            print(f"  {rel}")
            print(f"    moods:  {moods}")
            print(f"    genres: {genres}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
