"""Read-only browsing of content/presets/ and content/wavetables/ -- Part A's
Phase 1 tools (docs/MCP_AND_NL_PATCH_GENERATION.md) from server.py."""
from __future__ import annotations

import json
from pathlib import Path

from patch_schema import ENGINE_NAMES

REPO_ROOT = Path(__file__).resolve().parent.parent
PRESETS_DIR = REPO_ROOT / "content" / "presets"
WAVETABLES_DIR = REPO_ROOT / "content" / "wavetables"


def _array_contains(values: list, needle: str) -> bool:
    n = needle.strip().lower()
    return any(str(v).strip().lower() == n for v in values)


def list_presets(
    category: str | None = None,
    mood: str | None = None,
    genre: str | None = None,
    tag: str | None = None,
) -> list[dict]:
    """Every .pw8 under content/presets/, summarized. Filters combine with AND
    (same semantics as the plugin PresetMetadataFilter). Context/genre also
    matches legacy values stored in moods[] (e.g. cinematic)."""
    out = []
    for path in sorted(PRESETS_DIR.rglob("*.pw8")):
        try:
            data = json.loads(path.read_text())
        except (json.JSONDecodeError, OSError):
            continue
        meta = data.get("metadata", {})
        cat = meta.get("category") or path.parent.name
        moods = meta.get("moods", [])
        genres = meta.get("genres", [])
        tags = meta.get("tags", [])

        if category and category.lower() not in str(cat).lower():
            continue
        if mood and not _array_contains(moods, mood):
            continue
        if genre and not _array_contains(genres, genre) and not _array_contains(moods, genre):
            continue
        if tag and not _array_contains(tags, tag):
            continue

        engines = sorted({ENGINE_NAMES.get(op.get("engine", 0), "?")
                          for op in data.get("layerA", {}).get("operators", [])
                          if op.get("level", 0.0) > 0.0})
        out.append({
            "path": str(path.relative_to(REPO_ROOT)),
            "name": meta.get("name", path.stem),
            "category": cat,
            "moods": moods,
            "genres": genres,
            "tags": tags,
            "description": meta.get("description", ""),
            "engines": engines,
        })
    return out


def list_wavetables() -> list[dict]:
    """content/wavetables/*.json, with each table's frame count (a rough
    'static texture vs. long evolving morph' signal) and category inferred
    from its filename prefix (ambient-/bass-/bell-/etc.)."""
    out = []
    for path in sorted(WAVETABLES_DIR.glob("*.json")):
        try:
            data = json.loads(path.read_text())
        except (json.JSONDecodeError, OSError):
            continue
        prefix = path.stem.split("-")[0]
        out.append({
            "id": f"content/wavetables/{path.name}",
            "name": path.stem,
            "category": prefix,
            "numFrames": data.get("numFrames"),
        })
    return out


def read_patch_file(path: str) -> dict:
    full = (REPO_ROOT / path) if not Path(path).is_absolute() else Path(path)
    if not full.exists():
        raise FileNotFoundError(f"no patch at {path}")
    return json.loads(full.read_text())
