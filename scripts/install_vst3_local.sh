#!/usr/bin/env bash
# scripts/install_vst3_local.sh — copy a fresh MURMUR VST3 into ~/Library.
#
# Usage:
#   scripts/install_vst3_local.sh [path/to/MURMUR.vst3]
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# shellcheck source=scripts/lib/murmur_deploy_lib.sh
source "${REPO_ROOT}/scripts/lib/murmur_deploy_lib.sh"

CONFIG="${PW8_VST3_CONFIG:-Release}"
DEST="${HOME}/Library/Audio/Plug-Ins/VST3/MURMUR.vst3"

if [[ $# -ge 1 ]]; then
  SRC="$1"
else
  SRC="$(murmur_newest_artefact vst3 "$CONFIG" "$REPO_ROOT" || true)"
  if [[ -z "$SRC" ]]; then
    echo "ERROR: MURMUR.vst3 not found. Build first:" >&2
    echo "  cmake --preset plugin-release && cmake --build --preset plugin-release --target pw8_plugin_VST3" >&2
    exit 1
  fi
fi

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: not a bundle: $SRC" >&2
  exit 1
fi

SRC_VER="$(plutil -extract CFBundleShortVersionString raw "${SRC}/Contents/Info.plist" 2>/dev/null || echo '?')"
SRC_BUILD="$(plutil -extract CFBundleVersion raw "${SRC}/Contents/Info.plist" 2>/dev/null || echo '?')"

echo "==> Install VST3"
echo "    from: $SRC"
echo "    to:   $DEST"
echo "    source version: ${SRC_VER} (${SRC_BUILD})"

if [[ -d "$DEST" ]]; then
  OLD_VER="$(plutil -extract CFBundleShortVersionString raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
  OLD_BUILD="$(plutil -extract CFBundleVersion raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
  echo "    replacing: ${OLD_VER} (${OLD_BUILD})"
fi

mkdir -p "$(dirname "$DEST")"
rsync -a --delete "${SRC}/" "${DEST}/"

VER="$(plutil -extract CFBundleShortVersionString raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
BUILD="$(plutil -extract CFBundleVersion raw "${DEST}/Contents/Info.plist" 2>/dev/null || echo '?')"
echo "    installed: ${VER} (${BUILD})"

if [[ "$VER" != "$SRC_VER" || "$BUILD" != "$SRC_BUILD" ]]; then
  echo "ERROR: installed VST3 version mismatch (expected ${SRC_VER}/${SRC_BUILD}, got ${VER}/${BUILD})" >&2
  exit 1
fi

echo ""
echo "VST3 ready at: ${DEST}"
