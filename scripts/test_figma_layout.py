#!/usr/bin/env python3
"""Unit tests for scripts/figma_layout — run: python3 scripts/test_figma_layout.py"""
from __future__ import annotations

import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))

from figma_layout.annotate import suggest_annotations  # noqa: E402
from figma_layout.check import check_all  # noqa: E402
from figma_layout.compare import compare_metadata_to_layout  # noqa: E402
from figma_layout.cpp_constants import parse_play_mode_layout  # noqa: E402
from figma_layout.drift import validate_drift_bindings  # noqa: E402
from figma_layout.geometry import validate_column_layouts, validate_parent_links, validate_stack_geometry  # noqa: E402
from figma_layout.metadata_parser import metadata_to_layout_spec, parse_metadata_xml  # noqa: E402
from figma_layout.paths import LAYOUTS_DIR, PLAY_MODE_LAYOUT_H  # noqa: E402
from figma_layout.policies import load_policy_index, policy_for_spec, registry_pending  # noqa: E402
from figma_layout.report import generate_report  # noqa: E402
from figma_layout.schema_check import validate_against_schema  # noqa: E402
from figma_layout.snippet import generate_snippet  # noqa: E402
from figma_layout.spec import load_layout  # noqa: E402
from figma_layout.validate import exit_code  # noqa: E402


FIXTURES = Path(__file__).parent / "figma_layout" / "fixtures"
COMPACT_LAYOUT = LAYOUTS_DIR / "murmur-compact-view.4-1134.layout.json"
COMPACT_XML = FIXTURES / "murmur-compact-view.metadata.xml"


class FigmaLayoutTests(unittest.TestCase):
    def test_parse_play_mode_layout_has_compact_constants(self) -> None:
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        self.assertEqual(cpp["kCompactWidth"], 320)
        self.assertEqual(cpp["kCompactDefaultHeight"], 560)
        self.assertEqual(cpp["kCompactScopePanelHeight"], 152)

    def test_policies_loaded_for_all_exported_frames(self) -> None:
        index = load_policy_index()
        for name in (
            "murmur-play-compact",
            "murmur-design-fx",
            "murmur-mod-matrix",
            "murmur-basic-view",
            "master-envelope-panel",
            "murmur-preset-browser",
            "murmur-engine-deep-editor",
            "murmur-dual-lfo-lab",
        ):
            self.assertIn(name, index)
            self.assertIn("tier", index[name])

    def test_registry_pending_is_empty(self) -> None:
        pending = registry_pending()
        self.assertEqual(pending, [])

    def test_compact_geometry_passes(self) -> None:
        spec = load_layout(COMPACT_LAYOUT)
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [], msg="\n".join(i.message for i in errors))

    def test_basic_view_column_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-basic-view.86-4.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        errors = [i for i in validate_column_layouts(spec, cpp, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_mod_matrix_column_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-mod-matrix.27-265.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        errors = [i for i in validate_column_layouts(spec, cpp, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_master_envelope_parent_link_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "master-envelope-panel.82-4.layout.json")
        errors = [i for i in validate_parent_links(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_drift_bindings_mod_matrix_grid_card(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-mod-matrix.27-265.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        errors = [i for i in validate_drift_bindings(spec, cpp, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_schema_validation_passes_for_compact(self) -> None:
        spec = load_layout(COMPACT_LAYOUT)
        errors = [i for i in validate_against_schema(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_metadata_export_matches_committed_bounds(self) -> None:
        spec = load_layout(COMPACT_LAYOUT)
        xml = COMPACT_XML.read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_compare_top_level_mode(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-basic-view.86-4.layout.json")
        xml = (FIXTURES / "murmur-basic-view.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec, mode="top-level")
        self.assertEqual(diffs, [])

    def test_metadata_parser_strips_prose_prefix(self) -> None:
        xml = (FIXTURES / "murmur-design-engine.metadata.xml").read_text(encoding="utf-8")
        prose = "Currently selected nodes:\n- 37:787\n\n" + xml
        root = parse_metadata_xml(prose)
        self.assertEqual(root.node_id, "37:787")

    def test_metadata_parser_frame_size(self) -> None:
        root = parse_metadata_xml(COMPACT_XML.read_text(encoding="utf-8"))
        spec = metadata_to_layout_spec(
            root,
            figma_file_key="PFt0LG6XmOiZWcSoUXIWIg",
            node_id="4:1134",
        )
        self.assertEqual(spec["size"]["width"], 320)
        self.assertEqual(spec["size"]["height"], 560)
        self.assertEqual(len(spec["children"]), 5)

    def test_annotate_suggests_code_map_for_header(self) -> None:
        spec = {
            "canonicalName": "murmur-basic-view",
            "children": [{"name": "header-bar", "height": 60, "width": 1248}],
        }
        suggestions = suggest_annotations(spec)
        fields = {(s.path, s.field) for s in suggestions}
        self.assertIn(("root.header-bar", "codeMap"), fields)

    def test_snippet_uses_policy_gap_key(self) -> None:
        text = generate_snippet(LAYOUTS_DIR / "murmur-basic-view.86-4.layout.json")
        self.assertIn("kMurmurBasicViewMainBodyGap", text)

    def test_snippet_width_constant_uses_width_dimension(self) -> None:
        text = generate_snippet(LAYOUTS_DIR / "murmur-design-mod-matrix.27-265.layout.json")
        self.assertIn("kDesignModMatrixPageSidebarWidth = 280", text)

    def test_report_lists_all_exported_frames(self) -> None:
        report = generate_report()
        for name in (
            "murmur-preset-browser",
            "murmur-engine-deep-editor",
            "murmur-dual-lfo-lab",
        ):
            self.assertIn(name, report)

    def test_game_time_check_passes(self) -> None:
        issues = check_all(strict=False)
        errors = [i for i in issues if i.level == "error"]
        self.assertEqual(errors, [], msg="\n".join(f"{i.path}: {i.message}" for i in errors))
        self.assertEqual(exit_code(issues), 0)

    def test_glow_ring_scale_ladder_matches_cpp(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "glow-ring-knobs.21-4.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        for entry in spec.get("scaleLadder") or []:
            const = entry.get("constant")
            px = entry.get("figmaPx")
            if not const or not re.match(r"^k[A-Z]", str(const)):
                continue
            self.assertIn(const, cpp, msg=f"missing {const}")
            self.assertEqual(cpp[const], px, msg=f"{const} drift")

    def test_design_fx_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-fx.35-4.layout.json")
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_play_view_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-play-view.36-4.layout.json")
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_design_fx_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-fx.35-4.layout.json")
        xml = (FIXTURES / "murmur-design-fx.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_play_view_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-play-view.36-4.layout.json")
        xml = (FIXTURES / "murmur-play-view.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_design_engine_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-engine.37-787.layout.json")
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_mod_matrix_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-mod-matrix.27-265.layout.json")
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_basic_view_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-basic-view.86-4.layout.json")
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_design_engine_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-engine.37-787.layout.json")
        xml = (FIXTURES / "murmur-design-engine.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_mod_matrix_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-mod-matrix.27-265.layout.json")
        xml = (FIXTURES / "murmur-design-mod-matrix.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_basic_view_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-basic-view.86-4.layout.json")
        xml = (FIXTURES / "murmur-basic-view.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_master_envelope_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "master-envelope-panel.82-4.layout.json")
        xml = (FIXTURES / "master-envelope-panel.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_width_constants_bind_to_width_not_height(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-design-mod-matrix.27-265.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        errors = [i for i in validate_drift_bindings(spec, cpp, "test") if i.level == "error"]
        self.assertEqual(errors, [])
        policy = policy_for_spec(spec)
        self.assertEqual(policy.get("tier"), "exported")

    def test_preset_browser_column_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-preset-browser.27-6.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        errors = [i for i in validate_column_layouts(spec, cpp, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_engine_deep_editor_column_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-engine-deep-editor.28-4.layout.json")
        cpp = parse_play_mode_layout(PLAY_MODE_LAYOUT_H)
        errors = [i for i in validate_column_layouts(spec, cpp, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_dual_lfo_lab_geometry_passes(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-dual-lfo-lab.15-247.layout.json")
        errors = [i for i in validate_stack_geometry(spec, "test") if i.level == "error"]
        self.assertEqual(errors, [])

    def test_preset_browser_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-preset-browser.27-6.layout.json")
        xml = (FIXTURES / "murmur-preset-browser.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_engine_deep_editor_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-engine-deep-editor.28-4.layout.json")
        xml = (FIXTURES / "murmur-engine-deep-editor.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))

    def test_dual_lfo_lab_metadata_matches_top_level(self) -> None:
        spec = load_layout(LAYOUTS_DIR / "murmur-dual-lfo-lab.15-247.layout.json")
        xml = (FIXTURES / "murmur-dual-lfo-lab.metadata.xml").read_text(encoding="utf-8")
        diffs = compare_metadata_to_layout(xml, spec)
        self.assertEqual(diffs, [], msg="\n".join(str(d) for d in diffs))


if __name__ == "__main__":
    unittest.main()
