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
# Default artefact: pick the newest Release AU among known build outputs (plugin-release first).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${PW8_AU_CONFIG:-Release}"
DEST="${HOME}/Library/Audio/Plug-Ins/Components/MURMUR.component"

if [[ $# -ge 1 ]]; then
  SRC="$1"
else
  SRC=""
  SRC_MTIME=0
  for candidate in \
    "build/plugin-release/plugin/pw8_plugin_artefacts/${CONFIG}/AU/MURMUR.component" \
    "build/plugin/pw8_plugin_artefacts/${CONFIG}/AU/MURMUR.component" \
    "build/plugin/plugin/pw8_plugin_artefacts/Debug/AU/MURMUR.component" \
    "build/dev/plugin/pw8_plugin_artefacts/Debug/AU/MURMUR.component"
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
    echo "ERROR: MURMUR.component not found. Build first:" >&2
    echo "  cmake --preset plugin-release && cmake --build --preset plugin-release --target pw8_plugin_AU" >&2
    exit 1
  fi
fi

if [[ ! -d "$SRC" ]]; then
  echo "ERROR: not a bundle: $SRC" >&2
  exit 1
fi

SRC_VER="$(plutil -extract CFBundleShortVersionString raw "${SRC}/Contents/Info.plist" 2>/dev/null || echo '?')"
SRC_BUILD="$(plutil -extract CFBundleVersion raw "${SRC}/Contents/Info.plist" 2>/dev/null || echo '?')"

echo "==> Install AU"
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
  echo "ERROR: installed AU version mismatch (expected ${SRC_VER}/${SRC_BUILD}, got ${VER}/${BUILD})" >&2
  exit 1
fi

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

PRESETS_SRC="${REPO_ROOT}/content/presets/factory"
PRESETS_DEST="${HOME}/Library/Application Support/MURMUR/Presets/factory"
if [[ -d "$PRESETS_SRC" ]]; then
  echo "==> Install factory presets"
  echo "    from: $PRESETS_SRC"
  echo "    to:   $PRESETS_DEST"
  mkdir -p "$PRESETS_DEST"
  rsync -a "${PRESETS_SRC}/" "${PRESETS_DEST}/"
  PRESET_COUNT=$(find "$PRESETS_DEST" \( -name '*.pw8' -o -name '*.murmur' \) | wc -l | tr -d ' ')
  SIDECHAIN_COUNT=$(find "$PRESETS_DEST/Sidechain" \( -name '*.pw8' -o -name '*.murmur' \) 2>/dev/null | wc -l | tr -d ' ')
  echo "    ${PRESET_COUNT} factory presets (${SIDECHAIN_COUNT} Sidechain / vocoder)"
fi

echo ""
echo "Logic Pro: quit Logic completely, reopen, then Plug-in Manager → Reset & Rescan Selection"
echo "           (or hold Option while opening Plug-in Manager → Reset & Rescan Selection)."
echo "Presets: open MURMUR preset browser → category filter \"sidechain\" or search \"vocoder\"."
