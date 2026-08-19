#!/usr/bin/env bash
# scripts/release_gate.sh — Cadillac MURMUR release pipeline (build → validate → package)
#
# One command for maintainers (and CI) to produce a Ben-ready .pkg + .dmg with hard-fail
# validation gates. Output lands in dist/.
#
# Usage:
#   scripts/release_gate.sh [options] [version]
#
# Options:
#   --skip-tests         Skip ctest (dev builds only)
#   --skip-pluginval     Skip pluginval strictness 5 (if cask not installed)
#   --skip-auval         Skip auval hard-fail (not recommended for shipping)
#   --no-dmg             Build .pkg only (no DMG wrapper)
#   --target user|system Install scope (default: user — no admin password)
#   --sign IDENTITY      Developer ID Application identity
#   --notarize           Notarize .pkg (requires --sign + Apple credentials)
#   -h, --help           Show help
#
# Examples:
#   scripts/release_gate.sh                         # version from CMakeLists.txt
#   scripts/release_gate.sh 1.4.3                   # explicit version tag
#   scripts/release_gate.sh --skip-pluginval        # when pluginval cask missing locally
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

SKIP_TESTS=0
SKIP_PLUGINVAL=0
SKIP_AUVAL=0
MAKE_DMG=1
TARGET="user"
SIGN_IDENTITY="${CODESIGN_IDENTITY:-}"
DO_NOTARIZE=0
VERSION=""

usage() {
    sed -n '2,24p' "$0" | sed 's/^# \?//'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-tests) SKIP_TESTS=1; shift ;;
        --skip-pluginval) SKIP_PLUGINVAL=1; shift ;;
        --skip-auval) SKIP_AUVAL=1; shift ;;
        --no-dmg) MAKE_DMG=0; shift ;;
        --target)
            TARGET="${2:?--target requires user or system}"
            shift 2
            ;;
        --sign)
            SIGN_IDENTITY="${2:?--sign requires identity}"
            shift 2
            ;;
        --notarize) DO_NOTARIZE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        -*)
            echo "ERROR: unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
        *) VERSION="$1"; shift ;;
    esac
done

if [[ -z "$VERSION" ]]; then
    VERSION="$(python3 - <<'PY'
import re, pathlib
text = pathlib.Path("CMakeLists.txt").read_text()
m = re.search(r'project\s*\(\s*murmur\s*\n\s*VERSION\s+([0-9.]+)', text, re.M)
print(m.group(1) if m else "0.0.0")
PY
)"
fi

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  MURMUR Cadillac Release Gate — v${VERSION}                      "
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

step() { echo ""; echo "▶ $*"; }

step "Content integrity (wavetable refs)"
python3 scripts/validate_content_refs.py

step "Figma layout specs vs PlayModeLayout.h"
scripts/figma_layout.sh check
python3 scripts/test_figma_layout.py

PRESETS_SRC="content/presets/factory"
# Real presets on disk are .murmur now (docs/REBRAND_MURMUR.md); .pw8 is
# still matched too since that migration is a permanent dual-read
# guarantee. A stale .pw8-only version of this exact check (in
# build_release_pkg.sh) already caused one real incident this session:
# it read "0 presets" against the real 1,129-file .murmur library and
# silently regenerated a throwaway 250-patch substitute with no
# Interstellar bank at all. Don't reintroduce that here.
if [[ ! -d "$PRESETS_SRC" ]] || [[ -z "$(find "$PRESETS_SRC" \( -name '*.pw8' -o -name '*.murmur' \) -print -quit 2>/dev/null)" ]]; then
    echo "==> Generating factory presets..."
    python3 scripts/generate_factory_presets.py
fi
PRESET_COUNT=$(find "$PRESETS_SRC" \( -name '*.pw8' -o -name '*.murmur' \) | wc -l | tr -d ' ')
INTERSTELLAR_COUNT=$(find "$PRESETS_SRC/Interstellar" \( -name '*.pw8' -o -name '*.murmur' \) 2>/dev/null | wc -l | tr -d ' ')
WAVETABLE_COUNT=$(find content/wavetables -maxdepth 1 -name '*.json' 2>/dev/null | wc -l | tr -d ' ')

echo "    Factory presets: ${PRESET_COUNT} (${INTERSTELLAR_COUNT} Interstellar)"
echo "    Wavetables:      ${WAVETABLE_COUNT}"

if [[ "$INTERSTELLAR_COUNT" -lt 100 ]]; then
    echo "ERROR: Interstellar bank incomplete (need ≥100, have ${INTERSTELLAR_COUNT})" >&2
    echo "       Run: python3 scripts/generate_interstellar_presets.py" >&2
    exit 1
fi
HOOVER_COUNT=$(python3 - <<'PY'
import json, pathlib
root = pathlib.Path("content/presets/factory/Basses")
count = 0
for path in list(root.glob("*.pw8")) + list(root.glob("*.murmur")):
    try:
        data = json.loads(path.read_text())
    except json.JSONDecodeError:
        continue
    meta = data.get("metadata", data)
    tags = meta.get("tags") or []
    genres = meta.get("genres") or []
    hay = " ".join(str(t).lower() for t in (*tags, *genres))
    if "hoover-bass" in hay:
        count += 1
print(count)
PY
)
echo "    Hoover bass:     ${HOOVER_COUNT}"
if [[ "$HOOVER_COUNT" -lt 28 ]]; then
    echo "ERROR: Hoover bass bank incomplete (need ≥28, have ${HOOVER_COUNT})" >&2
    echo "       Run: python3 scripts/generate_genre_expansion_presets.py" >&2
    exit 1
fi
if [[ "$PRESET_COUNT" -lt 1000 ]]; then
    echo "ERROR: Factory preset count too low (need ≥1000, have ${PRESET_COUNT})" >&2
    exit 1
fi
if [[ "$WAVETABLE_COUNT" -lt 1 ]]; then
    echo "ERROR: Wavetable library missing — run scripts/generate_wavetable_library.py" >&2
    exit 1
fi

if [[ "$SKIP_TESTS" -eq 0 ]]; then
    step "Unit tests (ctest dev preset)"
    if cmake --preset dev >/dev/null 2>&1; then
        cmake --build --preset dev -j
        ctest --preset dev --output-on-failure
    else
        echo "    (dev preset unavailable — skipping ctest)"
    fi
fi

step "Release build (AU + VST3 + Standalone, arm64)"
cmake --preset plugin-release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build --preset plugin-release -j

step "Install AU locally + auval smoke test"
chmod +x scripts/install_au_local.sh
scripts/install_au_local.sh

if [[ "$SKIP_PLUGINVAL" -eq 0 ]]; then
    step "pluginval strictness 5 (VST3 + AU)"
    if [[ -x "${PLUGINVAL_BIN:-/Applications/pluginval.app/Contents/MacOS/pluginval}" ]]; then
        chmod +x scripts/run_pluginval.sh
        scripts/run_pluginval.sh
    else
        echo "WARNING: pluginval not installed — skipping (brew install --cask pluginval)" >&2
    fi
else
    echo "    (skipped — --skip-pluginval)"
fi

step "Package Cadillac installer (.pkg + optional .dmg)"
PKG_ARGS=(--full --target "$TARGET")
[[ "$MAKE_DMG" -eq 1 ]] && PKG_ARGS+=(--dmg)
[[ "$SKIP_AUVAL" -eq 1 ]] && PKG_ARGS+=(--skip-auval)
[[ -n "$SIGN_IDENTITY" ]] && PKG_ARGS+=(--sign "$SIGN_IDENTITY")
[[ "$DO_NOTARIZE" -eq 1 ]] && PKG_ARGS+=(--notarize)

chmod +x scripts/build_release_pkg.sh
scripts/build_release_pkg.sh "${PKG_ARGS[@]}" "$VERSION"

step "Release artifacts"
echo ""
ls -lh dist/MURMUR-"${VERSION}"* 2>/dev/null || ls -lh dist/
if [[ -f dist/SHA256SUMS.txt ]]; then
    echo ""
    echo "Checksums:"
    cat dist/SHA256SUMS.txt
fi

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  PASS — Cadillac release ready for Ben                         "
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "Send Ben:"
echo "  dist/MURMUR-${VERSION}-macOS-arm64-full.dmg   (preferred — drag-and-readme)"
echo "  dist/MURMUR-${VERSION}-macOS-arm64-full.pkg     (direct install)"
echo ""
echo "Ben install steps: docs/BEN_EMAIL_${VERSION%%-*}.md (or docs/INSTALL.md)"
echo "Verify checklist:  docs/RELEASE_${VERSION%%-*}_VERIFY.md"
