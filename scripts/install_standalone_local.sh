#!/usr/bin/env bash
# scripts/install_standalone_local.sh — copy MURMUR.app into ~/Applications for local testing.
#
# The AU installer (install_au_local.sh) does NOT install the standalone app — only this script does.
#
# Usage:
#   scripts/install_standalone_local.sh [path/to/MURMUR.app]
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${PW8_APP_CONFIG:-Release}"
DEST="${HOME}/Applications/MURMUR.app"

if [[ $# -ge 1 ]]; then
  SRC="$1"
else
  SRC=""
  SRC_MTIME=0
  for candidate in \
    "build/pw8_plugin_artefacts/${CONFIG}/Standalone/MURMUR.app" \
    "build/plugin/pw8_plugin_artefacts/${CONFIG}/Standalone/MURMUR.app" \
    "build/plugin-release/plugin/pw8_plugin_artefacts/${CONFIG}/Standalone/MURMUR.app" \
    "build/plugin/plugin/pw8_plugin_artefacts/Debug/Standalone/MURMUR.app"
  do
    [[ -d "$candidate" ]] || continue
    binary="${candidate}/Contents/MacOS/MURMUR"
    [[ -f "$binary" ]] || continue
    mtime=$(stat -f '%m' "$binary" 2>/dev/null || stat -c '%Y' "$binary" 2>/dev/null || echo 0)
    if [[ "$mtime" -gt "$SRC_MTIME" ]]; then
      SRC="$candidate"
      SRC_MTIME="$mtime"
    fi
  done

  if [[ -z "$SRC" ]]; then
    echo "ERROR: MURMUR.app not found. Build first:" >&2
    echo "  cmake --preset plugin-release && cmake --build --preset plugin-release --target pw8_plugin_Standalone" >&2
    exit 1
  fi
fi

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: not an app bundle: $SRC" >&2
  exit 1
fi

SRC_VER="$(plutil -extract CFBundleShortVersionString raw "${SRC}/Contents/Info.plist" 2>/dev/null || echo '?')"
SRC_BUILD="$(plutil -extract CFBundleVersion raw "${SRC}/Contents/Info.plist" 2>/dev/null || echo '?')"

echo "==> Install Standalone app"
echo "    from: $SRC"
echo "    to:   $DEST"
echo "    source version: ${SRC_VER} (${SRC_BUILD})"

if [[ -d "$DEST" ]]; then
  OLD_VER="$(plutil -extract CFBundleShortVersionString raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
  echo "    replacing: ${OLD_VER}"
fi

mkdir -p "$(dirname "$DEST")"
rsync -a --delete "${SRC}/" "${DEST}/"

VER="$(plutil -extract CFBundleShortVersionString raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
BUILD="$(plutil -extract CFBundleVersion raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
echo "    installed: ${VER} (${BUILD})"

echo ""
echo "Launch: open \"${DEST}\""
echo "Or find MURMUR in Spotlight / ~/Applications"
