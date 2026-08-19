from __future__ import annotations

import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
LAYOUTS_DIR = REPO_ROOT / "plugin/src/ui/figma-connect/layouts"
FIGMA_CONNECT_DIR = REPO_ROOT / "plugin/src/ui/figma-connect"
UI_SRC_DIR = REPO_ROOT / "plugin/src/ui"
PLAY_MODE_LAYOUT_H = UI_SRC_DIR / "PlayModeLayout.h"
FIGMA_KNOB_TOKENS_H = UI_SRC_DIR / "theme/FigmaKnobTokens.h"
SCHEMA_PATH = LAYOUTS_DIR / "figma-layout-spec.schema.json"
MANIFEST_PATH = LAYOUTS_DIR / "manifest.json"
