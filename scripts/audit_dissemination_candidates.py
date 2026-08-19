#!/usr/bin/env python3
"""Audit factory presets for macroDissemination (PoliMATHS Spread) candidates.

Scans content/presets/ for patches that would benefit from per-note macro capture:
pads, ambient, cinematic, polyphonic held-chord textures with 2–3 macro KOINS.

Run from repo root:
    python3 scripts/audit_dissemination_candidates.py
    python3 scripts/audit_dissemination_candidates.py --enable-top 40

Audit summary is printed and written to content/presets/factory/Dissemination/AUDIT.md
(when --enable-top is used, candidates are patched in place).
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
from collections import defaultdict

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
PRESETS_ROOT = REPO_ROOT / "content" / "presets"
AUDIT_DOC = REPO_ROOT / "content" / "presets" / "factory" / "Dissemination" / "AUDIT.md"

SRC_MACRO1 = 21
SRC_MACRO3 = 23

DISSEMINATION_TAGS = frozenset({
    "hymn-swell", "gran-cloud", "evolving-drone", "cosmic-pad", "wt-evolve",
    "dual-layer", "cinematic", "string-ensemble", "formant-choir", "wt-wash",
    "noise-drone", "fx-riser", "resonant-cave", "poly-sweet", "stack-showcase",
    "dual-layer", "granular", "dissemination",
})

PENALTY_TAGS = frozenset({
    "acid-seq", "mono-growl", "clock-tick", "pulsar", "singable-lead",
    "portamento", "sub-gravity", "fm-sub", "acid-line", "sync-lead",
})

PREFERRED_CATEGORIES = frozenset({"pad", "ambient", "interstellar"})


def score_preset(data: dict) -> tuple[int, dict]:
    vs = data.get("voiceSettings", {})
    if vs.get("macroDissemination"):
        return -1, {}

    meta = data.get("metadata", {})
    cat = meta.get("category") or "unknown"
    tags = set(meta.get("tags") or [])
    poly = int(vs.get("polyphony", 8))
    routes = data.get("layerA", {}).get("modRoutes", [])
    macro_routes = sum(1 for r in routes if SRC_MACRO1 <= r.get("source", 0) <= SRC_MACRO3)
    ui = data.get("uiFocus", {}).get("knobs", [])
    koin_count = sum(1 for k in ui if k.get("kind") == "macro")
    amp = (data.get("layerA", {}).get("envelopes") or [{}])[0]
    atk = float(amp.get("attackSeconds", 0))
    rel = float(amp.get("releaseSeconds", 0))

    score = 0
    if cat in PREFERRED_CATEGORIES:
        score += 30
    if poly >= 16:
        score += 15
    elif poly >= 12:
        score += 12
    elif poly >= 8:
        score += 8
    if macro_routes >= 6:
        score += 20
    elif macro_routes >= 4:
        score += 12
    elif macro_routes >= 2:
        score += 6
    if koin_count >= 3:
        score += 15
    elif koin_count >= 2:
        score += 12
    elif koin_count >= 1:
        score += 8
    if atk >= 1.0:
        score += 12
    elif atk >= 0.5:
        score += 8
    if rel >= 2.5:
        score += 12
    elif rel >= 1.5:
        score += 8
    if tags & DISSEMINATION_TAGS:
        score += 12
    if cat in ("bass", "seq") or (tags & PENALTY_TAGS):
        score -= 30
    if cat == "lead":
        score -= 15

    detail = {
        "category": cat,
        "polyphony": poly,
        "macro_routes": macro_routes,
        "koins": koin_count,
        "attack": round(atk, 3),
        "release": round(rel, 3),
        "name": meta.get("name", ""),
    }
    return score, detail


def scan_presets(min_score: int = 40) -> list[tuple[int, pathlib.Path, dict]]:
    results = []
    for pw8 in sorted(PRESETS_ROOT.rglob("*.pw8")) + sorted(PRESETS_ROOT.rglob("*.murmur")):
        if "Dissemination" in pw8.parts:
            continue
        try:
            data = json.loads(pw8.read_text())
        except json.JSONDecodeError:
            continue
        score, detail = score_preset(data)
        if score < min_score:
            continue
        rel = pw8.relative_to(PRESETS_ROOT)
        results.append((score, rel, detail))
    results.sort(key=lambda x: (-x[0], str(x[1])))
    return results


def top_by_category(candidates: list, per_cat: int = 20) -> dict[str, list]:
    by_cat: dict[str, list] = defaultdict(list)
    for score, rel, detail in candidates:
        cat = detail["category"]
        if len(by_cat[cat]) < per_cat:
            by_cat[cat].append((score, rel, detail))
    return dict(sorted(by_cat.items()))


def format_audit(candidates: list, enabled: list[pathlib.Path] | None = None) -> str:
    by_cat = top_by_category(candidates)
    already = sum(1 for pw8 in list(PRESETS_ROOT.rglob("*.pw8")) + list(PRESETS_ROOT.rglob("*.murmur"))
                    if json.loads(pw8.read_text()).get("voiceSettings", {}).get("macroDissemination"))
    total = sum(1 for _ in list(PRESETS_ROOT.rglob("*.pw8")) + list(PRESETS_ROOT.rglob("*.murmur")))

    lines = [
        "# Macro Dissemination Candidate Audit",
        "",
        "PoliMATHS-inspired per-note macro capture (`voiceSettings.macroDissemination: true`).",
        "Best for held chords, pads, and slow macro sweeps where each voice keeps its",
        "note-on macro snapshot while new notes track live macro movement.",
        "",
        "## Summary",
        "",
        f"| Metric | Count |",
        f"|--------|------:|",
        f"| Total `.pw8` scanned | {total} |",
        f"| Already have dissemination | {already} |",
        f"| Candidates (score ≥ 40) | {len(candidates)} |",
    ]
    if enabled:
        lines.append(f"| Enabled this run | {len(enabled)} |")

    cat_counts: dict[str, int] = defaultdict(int)
    for _, _, d in candidates:
        cat_counts[d["category"]] += 1
    lines += ["", "### Candidates by category", ""]
    for cat, n in sorted(cat_counts.items(), key=lambda x: -x[1]):
        lines.append(f"- **{cat}**: {n}")

    lines += ["", "## Top 20 by category", ""]
    for cat, items in by_cat.items():
        lines.append(f"### {cat.title()} ({len(items)} shown)")
        lines.append("")
        lines.append("| Score | Preset | Macro routes | KOINS | Poly |")
        lines.append("|------:|--------|-------------:|------:|-----:|")
        for score, rel, d in items:
            lines.append(
                f"| {score} | `{rel}` | {d['macro_routes']} | {d['koins']} | {d['polyphony']} |"
            )
        lines.append("")

    if enabled:
        lines += ["## Enabled on existing presets", ""]
        for p in sorted(enabled):
            lines.append(f"- `{p.relative_to(PRESETS_ROOT)}`")
        lines.append("")

    lines += [
        "## Scoring criteria",
        "",
        "- Category bonus: pad, ambient, interstellar (+30)",
        "- Polyphony ≥ 8 (+8–15), macro routes ≥ 2 (+6–20), KOINS ≥ 1 (+8–15)",
        "- Slow attack/release (+8–12), dissemination-friendly tags (+12)",
        "- Penalty: bass, seq, mono-growl, pulsar, lead-oriented tags",
        "",
        "Regenerate: `python3 scripts/audit_dissemination_candidates.py`",
    ]
    return "\n".join(lines) + "\n"


def enable_dissemination(path: pathlib.Path) -> bool:
    data = json.loads(path.read_text())
    vs = data.setdefault("voiceSettings", {})
    if vs.get("macroDissemination"):
        return False
    vs["macroDissemination"] = True
    tags = data.setdefault("metadata", {}).setdefault("tags", [])
    if "dissemination" not in tags:
        tags.append("dissemination")
    path.write_text(json.dumps(data, indent=2) + "\n")
    return True


def pick_enable_set(candidates: list, limit: int) -> list[pathlib.Path]:
    """Prefer pad/ambient/interstellar; skip golden-unfriendly unless needed."""
    picked: list[pathlib.Path] = []
    seen: set[str] = set()
    for score, rel, detail in candidates:
        if detail["category"] not in PREFERRED_CATEGORIES:
            continue
        if detail["category"] == "interstellar":
            tags_path = PRESETS_ROOT / rel
            data = json.loads(tags_path.read_text())
            tags = set(data.get("metadata", {}).get("tags") or [])
            if tags & PENALTY_TAGS:
                continue
        key = str(rel)
        if key in seen:
            continue
        seen.add(key)
        picked.append(PRESETS_ROOT / rel)
        if len(picked) >= limit:
            break
    return picked


def main():
    parser = argparse.ArgumentParser(description="Audit macro dissemination candidates")
    parser.add_argument("--min-score", type=int, default=40)
    parser.add_argument("--enable-top", type=int, default=0,
                        help="Enable macroDissemination on top N pad/ambient/interstellar candidates")
    args = parser.parse_args()

    candidates = scan_presets(args.min_score)
    enabled: list[pathlib.Path] = []

    if args.enable_top > 0:
        for path in pick_enable_set(candidates, args.enable_top):
            if enable_dissemination(path):
                enabled.append(path)
        print(f"Enabled macroDissemination on {len(enabled)} presets.")

    AUDIT_DOC.parent.mkdir(parents=True, exist_ok=True)
    report = format_audit(candidates, enabled or None)
    AUDIT_DOC.write_text(report)
    print(report)
    print(f"Wrote {AUDIT_DOC}")


if __name__ == "__main__":
    main()
