#!/usr/bin/env bash
# scripts/run_pluginval.sh — local pluginval soak (strictness 5) after plugin-release build.
#
# Requires: macOS, Homebrew cask pluginval, built MURMUR VST3 + AU artifacts.
# CI runs the same steps in .github/workflows/ci.yml (plugin job).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${PLUGINVAL_CONFIG:-Release}"
if [[ -n "${PLUGINVAL_ARTEFACT_ROOT:-}" ]]; then
  ARTEFACT_ROOT="$PLUGINVAL_ARTEFACT_ROOT"
elif [[ -d "build/plugin-release/plugin/pw8_plugin_artefacts/${CONFIG}" ]]; then
  ARTEFACT_ROOT="build/plugin-release/plugin/pw8_plugin_artefacts/${CONFIG}"
else
  ARTEFACT_ROOT="build/plugin/plugin/pw8_plugin_artefacts/${CONFIG}"
fi
VST3="${ARTEFACT_ROOT}/VST3/MURMUR.vst3"
AU="${ARTEFACT_ROOT}/AU/MURMUR.component"
PLUGINVAL="${PLUGINVAL_BIN:-/Applications/pluginval.app/Contents/MacOS/pluginval}"

if [[ ! -x "$PLUGINVAL" ]]; then
  echo "pluginval not found at $PLUGINVAL" >&2
  echo "Install: brew install --cask pluginval" >&2
  exit 1
fi

if [[ ! -d "$VST3" ]]; then
  echo "VST3 not found — build first:" >&2
  echo "  cmake --preset plugin-release && cmake --build --preset plugin-release" >&2
  exit 1
fi

echo "==> Register AU for validation"
mkdir -p ~/Library/Audio/Plug-Ins/Components
cp -R "$AU" ~/Library/Audio/Plug-Ins/Components/
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "==> auval (smoke)"
auval -v aumu Murm Murr

run_pluginval() {
  local label="$1"
  local target="$2"
  echo "==> pluginval strictness 5 — ${label}"
  local log
  log="$(mktemp)"
  if ! "$PLUGINVAL" --strictness-level 5 --validate "$target" 2>&1 | tee "$log"; then
    rm -f "$log"
    return 1
  fi
  if grep -q "Segmentation fault" "$log"; then
    echo "FAIL: pluginval crashed during validation (${label})" >&2
    rm -f "$log"
    return 1
  fi
  if ! grep -q "Completed tests in pluginval / Plugin state" "$log"; then
    echo "FAIL: pluginval did not finish Plugin state tests (${label})" >&2
    rm -f "$log"
    return 1
  fi
  rm -f "$log"
}

echo "==> pluginval strictness 5 — VST3"
run_pluginval "VST3" "$VST3"

echo "==> pluginval strictness 5 — AU"
run_pluginval "AU" "$AU"

echo "PASS: pluginval strictness 5 (VST3 + AU)"
