#!/usr/bin/env bash
# End-to-end Figma layout pipeline: gate, fixture compares, optional live fetch, report.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

LAYOUTS_DIR="plugin/src/ui/figma-connect/layouts"
FIXTURES_DIR="scripts/figma_layout/fixtures"

echo "==> sync gate"
./scripts/figma_layout_sync.sh

echo "==> fixture compare (top-level bounds)"
for spec in "$LAYOUTS_DIR"/*.layout.json; do
  base="$(basename "$spec" .layout.json)"
  fixture="$FIXTURES_DIR/${base%%.*}.metadata.xml"
  # fixture name uses canonical slug before first dot segment
  slug="${base%%.*}"
  fixture="$FIXTURES_DIR/${slug}.metadata.xml"
  if [[ -f "$fixture" ]]; then
    echo "  compare $base"
    scripts/figma_layout.sh compare "$(basename "$spec")" -i "$fixture" --mode top-level
  fi
done

if [[ -f .env.local ]] && grep -q 'FIGMA_ACCESS_TOKEN=' .env.local 2>/dev/null; then
  echo "==> live fetch compare"
  while IFS= read -r spec; do
    echo "  fetch $(basename "$spec")"
    scripts/figma_layout.sh fetch "$(basename "$spec")" --mode top-level
    sleep 1
  done < <(find "$LAYOUTS_DIR" -maxdepth 1 -name '*.layout.json' | sort)
else
  echo "==> skip live fetch (.env.local / FIGMA_ACCESS_TOKEN not set)"
fi

echo "==> report"
scripts/figma_layout.sh report --output docs/FIGMA_LAYOUT_REPORT.md

echo "OK: figma_layout e2e complete"
