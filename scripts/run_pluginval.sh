#!/usr/bin/env bash
# scripts/run_pluginval.sh — local pluginval soak (strictness 5) after plugin-release build.
#
# Requires: macOS, Homebrew cask pluginval, built MURMUR VST3 + AU artifacts.
# CI runs the same steps in .github/workflows/ci.yml (plugin job).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${PLUGINVAL_CONFIG:-Release}"
ARTEFACT_ROOT="build/plugin/plugin/pw8_plugin_artefacts/${CONFIG}"
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

echo "==> pluginval strictness 5 — VST3"
"$PLUGINVAL" --strictness-level 5 --validate "$VST3"

echo "==> pluginval strictness 5 — AU"
"$PLUGINVAL" --strictness-level 5 --validate "$AU"

echo "PASS: pluginval strictness 5 (VST3 + AU)"
