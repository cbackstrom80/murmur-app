#!/usr/bin/env bash
# scripts/mvp_check.sh — one-shot MVP regression gate for Patchwork Eight.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'
NC='\033[0m'

step() { echo -e "\n${BOLD}==> $*${NC}"; }
pass() { echo -e "${GREEN}PASS${NC}: $*"; }
fail() { echo -e "${RED}FAIL${NC}: $*" >&2; exit 1; }

FUZZ_COUNT="${MVP_FUZZ_COUNT:-200}"
FUZZ_SEED="${MVP_FUZZ_SEED:-42}"
SKIP_FUZZ="${MVP_SKIP_FUZZ:-0}"
SKIP_PLUGIN="${MVP_SKIP_PLUGIN:-0}"

step "Configure + build (dev preset)"
cmake --preset dev
cmake --build --preset dev -j

step "Run test suite (ctest)"
ctest --preset dev --output-on-failure
pass "All ctest cases passed"

step "Validate wavetable references in presets"
python3 scripts/validate_content_refs.py
pass "Content refs OK"

step "Validate Figma layout specs vs PlayModeLayout.h"
scripts/figma_layout.sh check
python3 scripts/test_figma_layout.py
pass "Figma layout specs OK"

step "MCP server smoke test"
python3 mcp_server/smoke_test.py
pass "MCP smoke OK"

RENDER_BIN="build/dev/tools/pw8-render"
[[ -x "$RENDER_BIN" ]] || fail "pw8-render not found at $RENDER_BIN"

step "Headless render smoke (aurora-showcase)"
PRESET="content/presets/aurora-showcase.pw8"
MIDI="content/test_midi/aurora-showcase-chords.mid"
[[ -f "$PRESET" ]] || PRESET="content/presets/fm-bell.pw8"
[[ -f "$MIDI" ]] || MIDI="content/test_midi/single-note.mid"
OUT_WAV="$(mktemp /tmp/pw8-mvp-XXXXXX.wav)"
OUT_JSON="$(mktemp /tmp/pw8-mvp-XXXXXX.json)"
"$RENDER_BIN" --patch "$PRESET" --midi "$MIDI" --sample-rate 48000 --bpm 90 \
  --output "$OUT_WAV" --receipt "$OUT_JSON"
python3 - <<PY
import json, sys
r = json.load(open("$OUT_JSON"))
m = r.get("metrics", {})
if m.get("containsNaNOrInf"):
    sys.exit("render receipt reports NaN/Inf")
peak = m.get("peak")
if peak is not None and float(peak) < 1e-6:
    sys.exit(f"render too quiet: peak={peak}")
print(f"peak={peak}")
PY
rm -f "$OUT_WAV" "$OUT_JSON"
pass "pw8-render OK"

if [[ "$SKIP_FUZZ" != "1" ]]; then
  step "Fuzz render sample (${FUZZ_COUNT} patches, seed=${FUZZ_SEED})"
  FUZZ_BIN="build/dev/tools/pw8-fuzz-render"
  [[ -x "$FUZZ_BIN" ]] || fail "pw8-fuzz-render not found"
  "$FUZZ_BIN" --count "$FUZZ_COUNT" --seed "$FUZZ_SEED"
  pass "Fuzz render OK"
else
  echo "Skipping fuzz (MVP_SKIP_FUZZ=1)"
fi

if [[ "$SKIP_PLUGIN" != "1" ]]; then
  step "Plugin shared library build (plugin preset)"
  cmake --preset plugin
  cmake --build --preset plugin -j --target pw8_plugin
  pass "pw8_plugin built"
else
  echo "Skipping plugin (MVP_SKIP_PLUGIN=1)"
fi

echo ""
echo -e "${GREEN}${BOLD}MVP CHECK PASSED${NC}"
echo "See docs/MVP.md for manual demo checklist + Patchforge integration."
