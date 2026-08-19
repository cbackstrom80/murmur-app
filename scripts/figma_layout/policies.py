from __future__ import annotations

import json
from typing import Any

from .manifest import load_manifest
from .paths import MANIFEST_PATH

# Default name → C++ owner hints for annotate.
CODE_MAP_HINTS: dict[str, str] = {
    "header-bar": "MurmurChromeBar",
    "status-bar": "VstBottomBar",
    "vst-bottom-bar": "VstBottomBar",
    "bottom-bar": "VstBottomBar",
    "master-envelope-panel": "MasterEnvelopePanel",
    "matrix-workspace": "DesignModMatrixPanel",
    "main-body": "PlayModeEditor",
    "grid-section": "EngineGridPanel",
    "fx-signal-chain-container": "DesignFxSignalChain",
    "envelope-shaping-panel": "MasterEnvelopePanel",
    "performance-sidebar": "PatchFocusPanel",
}

# Fallback policies when manifest entry lacks layoutPolicy (canonical → policy).
DEFAULT_POLICIES: dict[str, dict[str, Any]] = {
    "murmur-play-compact": {
        "tier": "required",
        "frameSize": ["kCompactWidth", "kCompactDefaultHeight"],
        "sectionGap": "kCompactBlockGap",
        "insets": {"top": "kCompactOuterMargin", "bottom": "kCompactBottomMargin"},
        "geometry": ["vertical-stack"],
        "cppOwners": ["CompactModeEditor.cpp", "MurmurChromeBar.cpp"],
    },
    "murmur-compact-view": {
        "tier": "required",
        "frameSize": ["kCompactWidth", "kCompactDefaultHeight"],
        "sectionGap": "kCompactBlockGap",
        "insets": {"top": "kCompactOuterMargin", "bottom": "kCompactBottomMargin"},
        "geometry": ["vertical-stack"],
        "cppOwners": ["CompactModeEditor.cpp"],
    },
    "glow-ring-knobs": {
        "tier": "required",
        "geometry": [],
        "validateScaleLadder": True,
        "validateHeroRatios": True,
        "cppOwners": ["GlowKnob.cpp", "DeckedKnobDraw.cpp"],
    },
    "murmur-design-fx": {
        "tier": "required",
        "frameSize": ["kDefaultWidth", "kDefaultHeight"],
        "sectionGap": "kDesignFxPageSectionGap",
        "insets": {"top": "kDesignModeV2OuterMargin", "bottom": "kDesignModeV2OuterMargin"},
        "geometry": ["vertical-stack"],
        "cppOwners": ["DesignFxPanel.cpp", "DesignFxSignalChain.cpp"],
    },
    "murmur-desktop-play-mode": {
        "tier": "required",
        "frameWidthOnly": True,
        "sectionGap": "kDesktopPlayModeSectionGap",
        "insets": {"top": "kDesktopPlayModeOuterMargin"},
        "geometry": ["vertical-stack"],
        "cppOwners": ["PlayModeEditor.cpp"],
    },
    "murmur-design-engine": {
        "tier": "exported",
        "frameWidthOnly": True,
        "sectionGap": "kDesignModeV2SectionGap",
        "insets": {"top": "kDesignModeV2OuterMargin", "bottom": "kDesignModeV2OuterMargin"},
        "geometry": ["vertical-stack"],
        "cppOwners": ["DesignModeEditor.cpp", "EngineGridPanel.cpp"],
    },
    "murmur-mod-matrix": {
        "tier": "exported",
        "frameSize": ["kDefaultWidth", "kDefaultHeight"],
        "sectionGap": "kDesignModMatrixPageSectionGap",
        "insets": {"top": "kDesignModMatrixPageOuterMargin", "bottom": "kDesignModMatrixPageOuterMargin"},
        "geometry": ["vertical-stack"],
        "columnLayouts": [
            {
                "parentName": "matrix-workspace",
                "direction": "horizontal",
                "gapConstant": "kDesignModMatrixPageSidebarGap",
                "leftChildren": ["matrix-grid-card", "active-routings-card"],
                "rightChildren": ["right-assign-panel"],
            }
        ],
        "cppOwners": ["DesignModMatrixPanel.cpp"],
    },
    "murmur-basic-view": {
        "tier": "exported",
        "frameSize": ["kDefaultWidth", "kDefaultHeight"],
        "sectionGap": "kMurmurBasicViewMainBodyGap",
        "insets": {"top": "kMurmurBasicViewOuterMargin", "bottom": "kMurmurBasicViewOuterMargin"},
        "geometry": ["vertical-stack"],
        "columnLayouts": [
            {
                "parentName": "main-body",
                "direction": "horizontal",
                "gapConstant": "kMurmurBasicViewMainBodyGap",
                "children": ["envelope-shaping-panel", "performance-sidebar"],
            }
        ],
        "cppOwners": ["PlayModeEditor.cpp"],
    },
    "master-envelope-panel": {
        "tier": "exported",
        "geometry": [],
        "parentLink": {"parentFrame": "37:787", "embedNodeId": "82:4"},
        "paddingConstant": "kDesignModeV2MasterEnvelopePadding",
        "cppOwners": ["MasterEnvelopePanel.cpp"],
    },
    "murmur-preset-browser": {
        "tier": "exported",
        "frameSize": ["kDefaultWidth", "kDefaultHeight"],
        "sectionGap": "kPresetBrowserPageColumnGap",
        "insets": {"top": "kPresetBrowserPageOuterMargin", "bottom": "kPresetBrowserPageOuterMargin"},
        "geometry": ["vertical-stack"],
        "columnLayouts": [
            {
                "parentName": "main-workspace",
                "direction": "horizontal",
                "gapConstant": "kPresetBrowserPageColumnGap",
                "children": ["left-categories", "center-presets-panel", "right-preset-detail"],
            }
        ],
        "cppOwners": ["PresetBrowserOverlay.cpp"],
    },
    "murmur-engine-deep-editor": {
        "tier": "exported",
        "frameSize": ["kEngineDeepEditorFrameWidth", "kEngineDeepEditorFrameHeight"],
        "sectionGap": "kEngineDeepEditorColumnGap",
        "insets": {"top": "kEngineDeepEditorOuterMargin", "bottom": "kEngineDeepEditorOuterMargin"},
        "geometry": ["vertical-stack"],
        "columnLayouts": [
            {
                "parentName": "main-content-row",
                "direction": "horizontal",
                "gapConstant": "kEngineDeepEditorColumnGap",
                "children": ["left-oscillator-column", "center-filter-column", "right-modulation-column"],
            }
        ],
        "cppOwners": ["EngineDetailOverlay.cpp"],
    },
    "murmur-dual-lfo-lab": {
        "tier": "exported",
        "frameSize": ["kDefaultWidth", "kDefaultHeight"],
        "sectionGap": "kDesignDualLfoPageSectionGap",
        "insets": {"top": "kDesignDualLfoPageOuterMargin", "bottom": "kDesignDualLfoPageOuterMargin"},
        "geometry": ["vertical-stack"],
        "cppOwners": ["DualLfoLabPanel.cpp"],
    },
}

# Constant prefix hints for annotate fuzzy match.
CONSTANT_PREFIX: dict[str, str] = {
    "murmur-play-compact": "kCompact",
    "murmur-compact-view": "kCompact",
    "murmur-design-fx": "kDesignFxPage",
    "murmur-desktop-play-mode": "kDesktopPlayMode",
    "murmur-design-engine": "kDesignModeV2",
    "murmur-mod-matrix": "kDesignModMatrixPage",
    "murmur-basic-view": "kMurmurBasicView",
    "master-envelope-panel": "kDesignModeV2MasterEnvelope",
}


def _merge_policy(base: dict[str, Any], override: dict[str, Any] | None) -> dict[str, Any]:
    if not override:
        return dict(base)
    out = dict(base)
    for key, val in override.items():
        if isinstance(val, dict) and isinstance(out.get(key), dict):
            merged = dict(out[key])
            merged.update(val)
            out[key] = merged
        else:
            out[key] = val
    return out


def load_policy_index() -> dict[str, dict[str, Any]]:
    """canonicalName → merged layout policy."""
    index: dict[str, dict[str, Any]] = {k: dict(v) for k, v in DEFAULT_POLICIES.items()}
    manifest = load_manifest()

    for entry in manifest.get("registry") or []:
        if not isinstance(entry, dict):
            continue
        canonical = entry.get("canonicalName")
        if not canonical:
            continue
        base = index.get(canonical, {})
        policy = entry.get("layoutPolicy") or {}
        tier = entry.get("tier") or entry.get("status")
        if tier and "tier" not in policy:
            policy = {**policy, "tier": tier}
        index[canonical] = _merge_policy(base, policy)

    for entry in manifest.get("layouts") or []:
        if not isinstance(entry, dict):
            continue
        canonical = entry.get("canonicalName")
        if not canonical:
            continue
        policy = entry.get("layoutPolicy") or {}
        index[canonical] = _merge_policy(index.get(canonical, {}), policy)

    return index


def policy_for_spec(spec: dict[str, Any]) -> dict[str, Any]:
    canonical = spec.get("canonicalName") or spec.get("frameName") or ""
    index = load_policy_index()
    return index.get(canonical, {})


def policy_tier(spec: dict[str, Any]) -> str:
    return str(policy_for_spec(spec).get("tier") or "exported")


def registry_pending() -> list[dict[str, Any]]:
    manifest = load_manifest()
    index = load_policy_index()
    pending: list[dict[str, Any]] = []
    exported_nodes = {e.get("nodeId") for e in manifest.get("layouts") or [] if isinstance(e, dict)}
    for reg in manifest.get("registry") or []:
        if not isinstance(reg, dict):
            continue
        canonical = reg.get("canonicalName", "")
        tier = reg.get("tier") or index.get(canonical, {}).get("tier") or reg.get("status")
        if tier == "pending" or (reg.get("required") and reg.get("nodeId") not in exported_nodes):
            pending.append(reg)
    return pending


def save_manifest_policy(canonical: str, layout_policy: dict[str, Any]) -> None:
    """Merge layoutPolicy into manifest registry entry (for annotate --write-policy)."""
    manifest = load_manifest()
    for reg in manifest.get("registry") or []:
        if isinstance(reg, dict) and reg.get("canonicalName") == canonical:
            reg["layoutPolicy"] = _merge_policy(reg.get("layoutPolicy") or {}, layout_policy)
            break
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
