#!/usr/bin/env bash
# One-shot Figma layout gate before UI work lands — validate, test, optional metadata diff.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "==> figma_layout check"
scripts/figma_layout.sh check "$@"

echo "==> figma_layout unit tests"
python3 scripts/test_figma_layout.py

if [[ -n "${FIGMA_METADATA_XML:-}" ]]; then
  echo "==> compare committed compact spec vs FIGMA_METADATA_XML"
  scripts/figma_layout.sh compare murmur-compact-view.4-1134.layout.json -i "$FIGMA_METADATA_XML"
fi

echo "OK: Figma layout toolchain game-time ready"
