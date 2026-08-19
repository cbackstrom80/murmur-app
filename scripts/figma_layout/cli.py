from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .annotate import apply_annotations, format_suggestions, suggest_annotations
from .check import check_all, exit_code, format_issues, print_summary
from .compare import compare_metadata_to_layout
from .fetch import fetch_and_compare, fetch_figma_node_metadata_xml
from .metadata_parser import metadata_to_layout_spec, parse_metadata_xml
from .paths import LAYOUTS_DIR, MANIFEST_PATH, REPO_ROOT
from .report import generate_report, write_report
from .snippet import generate_snippet
from .spec import layout_filename, load_layout, save_layout
from .validate import validate_all


def cmd_validate(args: argparse.Namespace) -> int:
    issues = validate_all(strict=args.strict)
    if issues:
        print(format_issues(issues), file=sys.stderr)
    else:
        print("figma_layout: all layout specs OK")
    return exit_code(issues)


def cmd_check(args: argparse.Namespace) -> int:
    issues = check_all(strict=args.strict)
    print_summary(issues)
    if issues:
        print(format_issues(issues), file=sys.stderr)
    else:
        print("figma_layout: game-time check passed")
    return exit_code(issues)


def cmd_export(args: argparse.Namespace) -> int:
    metadata_text = Path(args.input).read_text(encoding="utf-8") if args.input else sys.stdin.read()
    root = parse_metadata_xml(metadata_text)
    spec = metadata_to_layout_spec(
        root,
        figma_file_key=args.file_key,
        node_id=args.node_id,
        frame_name=args.name,
        export_source=args.source,
    )
    if args.annotate:
        suggestions = suggest_annotations(spec, tolerance=args.tolerance)
        spec = apply_annotations(spec, suggestions)
        if args.verbose:
            print(format_suggestions(suggestions), file=sys.stderr)
    out_name = args.output or layout_filename(spec["frameName"], args.node_id)
    out_path = LAYOUTS_DIR / out_name
    if args.dry_run:
        print(json.dumps(spec, indent=2))
        return 0
    save_layout(out_path, spec)
    print(f"Wrote {out_path.relative_to(REPO_ROOT)}")
    return 0


def cmd_compare(args: argparse.Namespace) -> int:
    metadata_text = Path(args.input).read_text(encoding="utf-8") if args.input else sys.stdin.read()
    layout_path = Path(args.layout)
    if not layout_path.is_file():
        layout_path = LAYOUTS_DIR / args.layout
    if not layout_path.is_file():
        print(f"layout not found: {args.layout}", file=sys.stderr)
        return 1
    spec = load_layout(layout_path)
    diffs = compare_metadata_to_layout(
        metadata_text,
        spec,
        mode=args.mode,
        tolerance=args.tolerance,
        strict=args.strict,
    )
    if not diffs:
        print(f"figma_layout compare: {layout_path.name} matches metadata bounds ({args.mode})")
        return 0
    for d in diffs:
        print(d)
    return 1


def cmd_snippet(args: argparse.Namespace) -> int:
    path = Path(args.layout)
    if not path.is_file():
        path = LAYOUTS_DIR / args.layout
    if not path.is_file():
        print(f"layout not found: {args.layout}", file=sys.stderr)
        return 1
    print(generate_snippet(path))
    return 0


def cmd_list(args: argparse.Namespace) -> int:
    if MANIFEST_PATH.is_file():
        print(MANIFEST_PATH.read_text(encoding="utf-8"))
        return 0
    for p in sorted(LAYOUTS_DIR.glob("*.layout.json")):
        spec = json.loads(p.read_text(encoding="utf-8"))
        print(f"{p.name}\t{spec.get('nodeId')}\t{spec.get('frameName')}")
    return 0


def cmd_annotate(args: argparse.Namespace) -> int:
    path = Path(args.layout)
    if not path.is_file():
        path = LAYOUTS_DIR / args.layout
    if not path.is_file():
        print(f"layout not found: {args.layout}", file=sys.stderr)
        return 1
    spec = load_layout(path)
    suggestions = suggest_annotations(spec, tolerance=args.tolerance)
    print(format_suggestions(suggestions))
    if args.write:
        updated = apply_annotations(spec, suggestions)
        save_layout(path, updated)
        print(f"Updated {path.relative_to(REPO_ROOT)}", file=sys.stderr)
    return 0


def cmd_fetch(args: argparse.Namespace) -> int:
    layout_path = args.layout
    if args.output:
        xml, diffs = fetch_and_compare(
            layout_path,
            file_key=args.file_key,
            node_id=args.node_id,
            mode=args.mode,
            tolerance=args.tolerance,
            strict=args.strict,
        )
        Path(args.output).write_text(xml, encoding="utf-8")
        print(f"Wrote metadata to {args.output}")
    else:
        path = LAYOUTS_DIR / layout_path if not Path(layout_path).is_file() else Path(layout_path)
        spec = load_layout(path)
        fk = args.file_key or spec.get("figmaFileKey", "")
        nid = args.node_id or spec.get("nodeId", "")
        xml = fetch_figma_node_metadata_xml(fk, nid, token=args.token)
        diffs = compare_metadata_to_layout(
            xml, spec, mode=args.mode, tolerance=args.tolerance, strict=args.strict
        )
    if not diffs:
        print(f"figma_layout fetch: {layout_path} matches live Figma ({args.mode})")
        return 0
    for d in diffs:
        print(d)
    return 1


def cmd_report(args: argparse.Namespace) -> int:
    text = generate_report()
    if args.output:
        out = Path(args.output)
        if not out.is_absolute():
            out = REPO_ROOT / out
        out.write_text(text, encoding="utf-8")
        print(f"Wrote {out.relative_to(REPO_ROOT)}")
    else:
        print(text)
    return 0


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="figma_layout",
        description="Figma layout export + PlayModeLayout validation toolchain",
    )
    sub = p.add_subparsers(dest="command", required=True)

    c = sub.add_parser("check", help="full game-time gate (validate + manifest + geometry + audit)")
    c.add_argument("--strict", action="store_true", help="fail on pending required registry frames")
    c.set_defaults(func=cmd_check)

    v = sub.add_parser("validate", help="validate committed layout JSON vs PlayModeLayout.h")
    v.add_argument("--strict", action="store_true", help="warn on unmapped constants + pending tiers")
    v.set_defaults(func=cmd_validate)

    e = sub.add_parser("export", help="convert get_metadata XML to layout.json")
    e.add_argument("--file-key", required=True, help="Figma file key")
    e.add_argument("--node-id", required=True, help="Root frame node id (e.g. 4:1134)")
    e.add_argument("--name", default=None, help="Override frame name slug")
    e.add_argument("--input", "-i", default=None, help="Metadata XML file (default: stdin)")
    e.add_argument("--output", "-o", default=None, help="Output filename under layouts/")
    e.add_argument("--source", default="figma_layout export", help="exportSource field value")
    e.add_argument("--dry-run", action="store_true", help="print JSON to stdout, do not write")
    e.add_argument("--annotate", action="store_true", help="apply annotation suggestions on export")
    e.add_argument("--tolerance", type=int, default=2, help="px tolerance for constant matching")
    e.add_argument("--verbose", action="store_true")
    e.set_defaults(func=cmd_export)

    cmp = sub.add_parser("compare", help="diff get_metadata XML bounds vs committed layout.json")
    cmp.add_argument("layout", help="layout json path or basename")
    cmp.add_argument("--input", "-i", required=True, help="Metadata XML file")
    cmp.add_argument(
        "--mode",
        choices=["bounds", "top-level", "top_level"],
        default="bounds",
        help="compare all bound nodes or top-level sections only",
    )
    cmp.add_argument("--tolerance", type=int, default=0, help="allowed px drift per dimension")
    cmp.add_argument("--strict", action="store_true", help="fail on nodes in metadata missing from JSON")
    cmp.set_defaults(func=cmd_compare)

    s = sub.add_parser("snippet", help="emit PlayModeLayout.h constant snippet from a layout file")
    s.add_argument("layout", help="layout json path or basename")
    s.set_defaults(func=cmd_snippet)

    a = sub.add_parser("annotate", help="suggest or apply constant/codeMap bindings")
    a.add_argument("layout", help="layout json path or basename")
    a.add_argument("--write", action="store_true", help="merge suggestions into layout JSON")
    a.add_argument("--tolerance", type=int, default=2)
    a.set_defaults(func=cmd_annotate)

    f = sub.add_parser("fetch", help="fetch live Figma node bounds and compare to committed JSON")
    f.add_argument("layout", help="layout json path or basename")
    f.add_argument("--file-key", default=None)
    f.add_argument("--node-id", default=None)
    f.add_argument("--token", default=None, help="Figma token (default: FIGMA_ACCESS_TOKEN env)")
    f.add_argument("--output", "-o", default=None, help="write fetched metadata XML to path")
    f.add_argument("--mode", choices=["bounds", "top-level", "top_level"], default="top-level")
    f.add_argument("--tolerance", type=int, default=0)
    f.add_argument("--strict", action="store_true")
    f.set_defaults(func=cmd_fetch)

    r = sub.add_parser("report", help="markdown summary of specs, tiers, and check issues")
    r.add_argument("--output", "-o", default=None, help="write report to file")
    r.set_defaults(func=cmd_report)

    l = sub.add_parser("list", help="list layout specs (manifest or directory scan)")
    l.set_defaults(func=cmd_list)

    return p


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
