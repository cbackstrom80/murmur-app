#!/usr/bin/env bash
# scripts/install_au_local.sh — copy a fresh MURMUR AU into ~/Library and force macOS rescan.
#
# Logic Pro (and Plug-in Manager) often keeps cached metadata when CFBundleVersion is
# unchanged. After rebuilding, bump project VERSION in CMakeLists.txt when Logic ignores
# overwrites at the same version string.
#
# Usage:
#   scripts/install_au_local.sh [path/to/MURMUR.component]
#
# Default artefact: build/plugin-release/.../Release/AU/MURMUR.component
# Falls back to:    build/plugin/plugin/.../Debug/AU/MURMUR.component
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${PW8_AU_CONFIG:-Release}"
DEST="${HOME}/Library/Audio/Plug-Ins/Components/MURMUR.component"

if [[ $# -ge 1 ]]; then
  SRC="$1"
elif [[ -d "build/plugin-release/plugin/pw8_plugin_artefacts/${CONFIG}/AU/MURMUR.component" ]]; then
  SRC="build/plugin-release/plugin/pw8_plugin_artefacts/${CONFIG}/AU/MURMUR.component"
elif [[ -d "build/plugin/plugin/pw8_plugin_artefacts/Debug/AU/MURMUR.component" ]]; then
  SRC="build/plugin/plugin/pw8_plugin_artefacts/Debug/AU/MURMUR.component"
else
  echo "ERROR: MURMUR.component not found. Build first:" >&2
  echo "  cmake --preset plugin-release && cmake --build --preset plugin-release" >&2
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
  if auval -v aumu Murm Murr >/dev/null 2>&1; then
    echo "    AU registered."
    break
  fi
  sleep 1
done

echo "==> auval"
if auval -v aumu Murm Murr; then
  echo "PASS: MURMUR AU installed and validated."
else
  echo "WARN: auval failed — quit Logic, run this script again, or rescan in Plug-in Manager." >&2
  exit 1
fi

echo ""
echo "Logic Pro: quit Logic completely, reopen, then Plug-in Manager → Reset & Rescan Selection"
echo "           (or hold Option while opening Plug-in Manager → Reset & Rescan Selection)."
