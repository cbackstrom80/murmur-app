#!/usr/bin/env bash
# scripts/install_quasar_au_local.sh — copy a fresh QUASAR AU into ~/Library and force macOS rescan.
#
# Usage:
#   scripts/install_quasar_au_local.sh [path/to/QUASAR.component]
#
# Default artefact: build/quasar-release/.../Release/AU/QUASAR.component
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${PW8_AU_CONFIG:-Release}"
DEST="${HOME}/Library/Audio/Plug-Ins/Components/QUASAR.component"

if [[ $# -ge 1 ]]; then
  SRC="$1"
elif [[ -d "build/quasar-release/quasar_plugin/pw8_quasar_plugin_artefacts/${CONFIG}/AU/QUASAR.component" ]]; then
  SRC="build/quasar-release/quasar_plugin/pw8_quasar_plugin_artefacts/${CONFIG}/AU/QUASAR.component"
else
  echo "ERROR: QUASAR.component not found. Build first:" >&2
  echo "  cmake --preset quasar-release && cmake --build --preset quasar-release" >&2
  exit 1
fi

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: not a bundle: $SRC" >&2
  exit 1
fi

echo "==> Install AU"
echo "    from: $SRC"
echo "    to:   $DEST"
mkdir -p "$(dirname "$DEST")"
rsync -a --delete "${SRC}/" "${DEST}/"

VER="$(plutil -extract CFBundleShortVersionString raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
BUILD="$(plutil -extract CFBundleVersion raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
echo "    version: ${VER} (${BUILD})"

echo "==> Clear AU cache + restart AudioComponentRegistrar"
rm -rf "${HOME}/Library/Caches/AudioUnitCache/com.apple.audiounits.cache" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true

echo "==> Wait for registrar rescan (up to 15s)"
for _ in $(seq 1 15); do
  if auval -v aufx Qsar Murr >/dev/null 2>&1; then
    echo "    AU registered."
    break
  fi
  sleep 1
done

echo "==> auval"
if auval -v aufx Qsar Murr; then
  echo "PASS: QUASAR AU installed and validated."
else
  echo "WARN: auval failed — quit Logic, run this script again, or rescan in Plug-in Manager." >&2
  exit 1
fi

PRESETS_SRC="${REPO_ROOT}/content/presets/quasar"
PRESETS_DEST="${HOME}/Library/Application Support/QUASAR/Presets/quasar"
if [[ -d "$PRESETS_SRC" ]]; then
  echo "==> Install factory presets"
  echo "    from: $PRESETS_SRC"
  echo "    to:   $PRESETS_DEST"
  mkdir -p "$PRESETS_DEST"
  rsync -a --delete "${PRESETS_SRC}/" "${PRESETS_DEST}/"
  PLAY_COUNT=$(find "${PRESETS_DEST}/play" -name '*.quasar' 2>/dev/null | wc -l | tr -d ' ')
  TOTAL_COUNT=$(find "$PRESETS_DEST" -name '*.quasar' | wc -l | tr -d ' ')
  echo "    ${TOTAL_COUNT} presets (${PLAY_COUNT} PLAY)"
fi

echo ""
echo "Logic Pro: quit Logic completely, reopen, then Plug-in Manager → Reset & Rescan Selection"
