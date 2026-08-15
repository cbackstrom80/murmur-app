#!/usr/bin/env bash
# scripts/build_release_quasar_pkg.sh — build QUASAR macOS release artifacts (.pkg, optional .dmg)
#
# Installs:
#   AU  -> ~/Library or /Library/Audio/Plug-Ins/Components/QUASAR.component
#   Factory .quasar presets -> Application Support/QUASAR/Presets/
#
# Usage:
#   scripts/build_release_quasar_pkg.sh [options] [version]
#
# Options:
#   --target user|system     Install scope (default: user)
#   --dmg                    Also create a drag-and-install .dmg wrapper
#   --sign IDENTITY          Code-sign bundles (Developer ID Application: …)
#   -h, --help               Show help
#
# Default version: 1.0.0 (QUASAR product version, not MURMUR project version)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

TARGET="user"
MAKE_DMG=0
SIGN_IDENTITY="${CODESIGN_IDENTITY:-}"
VERSION=""

usage() {
    sed -n '2,18p' "$0" | sed 's/^# \?//'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --target)
            TARGET="${2:?--target requires user or system}"
            shift 2
            ;;
        --dmg)
            MAKE_DMG=1
            shift
            ;;
        --sign)
            SIGN_IDENTITY="${2:?--sign requires an identity string}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            echo "ERROR: unknown option: $1" >&2
            usage >&2
            exit 1
            ;;
        *)
            VERSION="$1"
            shift
            ;;
    esac
done

if [[ "$TARGET" != "user" && "$TARGET" != "system" ]]; then
    echo "ERROR: --target must be 'user' or 'system' (got '$TARGET')" >&2
    exit 1
fi

if [[ -z "$VERSION" ]]; then
    VERSION="1.0.0"
fi

BUILD_DIR="build/quasar-release"
ARTEFACT_DIR="$BUILD_DIR/quasar_plugin/pw8_quasar_plugin_artefacts/Release"
STAGE_DIR="$BUILD_DIR/pkg-stage"
DIST_DIR="dist"
SCOPE_SUFFIX="arm64"
PKG_NAME="QUASAR-${VERSION}-macOS-${SCOPE_SUFFIX}.pkg"

AU_SRC="$ARTEFACT_DIR/AU/QUASAR.component"
PRESETS_SRC="content/presets/quasar"

if [[ "$TARGET" == "user" ]]; then
    INSTALL_BLURB="your home folder (no admin password):
  ~/Library/Audio/Plug-Ins/Components/QUASAR.component
  ~/Library/Application Support/QUASAR/Presets/"
    PRESET_INSTALL_PATH='~/Library/Application Support/QUASAR/Presets/quasar'
else
    INSTALL_BLURB="system-wide locations (admin password required):
  /Library/Audio/Plug-Ins/Components/QUASAR.component
  /Library/Application Support/QUASAR/Presets/"
    PRESET_INSTALL_PATH='/Library/Application Support/QUASAR/Presets/quasar'
fi

echo "==> QUASAR macOS release builder"
echo "    version: $VERSION"
echo "    target:  $TARGET ($SCOPE_SUFFIX pkg)"

if [[ ! -d "$AU_SRC" ]]; then
    echo "==> Release artifacts not found — building via quasar-release preset (arm64)..."
    cmake --preset quasar-release -DCMAKE_OSX_ARCHITECTURES=arm64
    cmake --build --preset quasar-release -j --target pw8_quasar_plugin_AU
fi

if [[ ! -d "$AU_SRC" ]]; then
    echo "ERROR: expected build artifact missing: $AU_SRC" >&2
    exit 1
fi

if [[ ! -d "$PRESETS_SRC" ]] || [[ -z "$(find "$PRESETS_SRC" -name '*.quasar' -print -quit 2>/dev/null)" ]]; then
    echo "ERROR: QUASAR presets missing under $PRESETS_SRC" >&2
    exit 1
fi
PRESET_COUNT=$(find "$PRESETS_SRC" -name '*.quasar' | wc -l | tr -d ' ')
INTERSTELLAR_COUNT=$(find "$PRESETS_SRC/interstellar" -name '*.quasar' 2>/dev/null | wc -l | tr -d ' ')
echo "==> Bundling $PRESET_COUNT .quasar presets (${INTERSTELLAR_COUNT} Interstellar companions)."

sign_bundle() {
    local path="$1"
    if [[ -n "$SIGN_IDENTITY" ]]; then
        codesign --force --deep --sign "$SIGN_IDENTITY" --options runtime --timestamp "$path"
    else
        codesign --force --deep --sign - --timestamp=none "$path"
    fi
    codesign --verify --deep --strict "$path"
}

echo "==> Code-signing bundles (${SIGN_IDENTITY:-ad-hoc})..."
sign_bundle "$AU_SRC"

echo "==> Staging install payloads ($TARGET)..."
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/components"

PAYLOAD="$STAGE_DIR/payload-root"
mkdir -p "$PAYLOAD/Library/Audio/Plug-Ins/Components"
mkdir -p "$PAYLOAD/Library/Application Support/QUASAR/Presets/quasar"
mkdir -p "$PAYLOAD/Library/Application Support/QUASAR/Docs"

cp -R "$AU_SRC" "$PAYLOAD/Library/Audio/Plug-Ins/Components/"
cp -R "$PRESETS_SRC/"* "$PAYLOAD/Library/Application Support/QUASAR/Presets/quasar/"

DOCS_DIR="$PAYLOAD/Library/Application Support/QUASAR/Docs"
for doc in QUASAR_STANDALONE_PLUGIN.md GLOBAL_QUASAR_FX_PLAN.md INSTALL.md; do
    if [[ -f "$REPO_ROOT/docs/$doc" ]]; then
        cp "$REPO_ROOT/docs/$doc" "$DOCS_DIR/"
    fi
done

mkdir -p "$PAYLOAD/Library/Application Support/QUASAR"
cat > "$PAYLOAD/Library/Application Support/QUASAR/README.txt" << README_EOF
QUASAR ${VERSION} — Binaural Spatial Effect
============================================

Logic Pro (Apple Silicon): AU Effects → Murmur → QUASAR

Factory presets (${PRESET_COUNT}):
  ${PRESET_INSTALL_PATH}/interstellar/

Pair with MURMUR Interstellar/Spatial .pw8 presets — each Spatial pad has a
companion .quasar scene (see metadata companionQuasar).

Workflow:
  1. MURMUR on instrument track
  2. QUASAR on master bus (after MURMUR)
  3. Load matching .quasar preset (e.g. 002-void-cathedral.quasar)
  4. Optional: route a bus to QUASAR Sidechain input

After installing: quit Logic, reopen, Plug-in Manager → Reset & Rescan.

Updates: https://github.com/cbackstrom80/patchwork-eight/releases
README_EOF

PKG_SCRIPTS="$STAGE_DIR/pkg-scripts"
mkdir -p "$PKG_SCRIPTS"
cp "$REPO_ROOT/scripts/pkg/postinstall" "$PKG_SCRIPTS/postinstall"
chmod +x "$PKG_SCRIPTS/postinstall"

pkgbuild --root "$PAYLOAD" \
    --identifier "com.patchwork.quasar.${SCOPE_SUFFIX}" \
    --version "$VERSION" \
    --install-location "/" \
    --scripts "$PKG_SCRIPTS" \
    "$STAGE_DIR/components/quasar.pkg" >/dev/null

RES_DIR="$STAGE_DIR/resources"
mkdir -p "$RES_DIR"

if [[ -n "$SIGN_IDENTITY" ]]; then
    SIGNING_BLURB="Developer ID signed"
else
    SIGNING_BLURB="ad-hoc signed (see docs/INSTALL.md for Gatekeeper)"
fi

cat > "$RES_DIR/welcome.txt" << WELCOME_EOF
QUASAR ${VERSION} — Binaural Spatial Effect for Logic Pro

This installer is built for Apple Silicon Macs and Logic Pro.
It places QUASAR in ${INSTALL_BLURB}

Included:
  • QUASAR Audio Unit (Logic: AU Effects → Murmur → QUASAR)
  • ${PRESET_COUNT} factory .quasar presets (${INTERSTELLAR_COUNT} Interstellar companions)
  • Setup docs (Application Support/QUASAR/Docs/)

After installing:
  1. Quit Logic completely and reopen
  2. Plug-in Manager → Reset & Rescan Selection
  3. Insert QUASAR on master bus after MURMUR
  4. Load a companion .quasar preset on headphones

Signing: ${SIGNING_BLURB}
WELCOME_EOF

cat > "$RES_DIR/conclusion.txt" << CONCLUSION_EOF
QUASAR is installed — ready for Logic Pro.

NEXT STEPS
----------
1. Quit and reopen Logic Pro.
2. Settings → Plug-in Manager → Reset & Rescan Selection.
3. Master bus → AU Effects → Murmur → QUASAR.
4. Load a companion preset from Application Support/QUASAR/Presets/quasar/interstellar/

MURMUR Spatial pads: load matching .pw8, then the companion .quasar for full binaural scene.

Updates: https://github.com/cbackstrom80/patchwork-eight/releases
CONCLUSION_EOF

cp "$REPO_ROOT/LICENSE" "$RES_DIR/LICENSE.txt"

if [[ "$TARGET" == "user" ]]; then
    DOMAIN_ATTR='enable_localSystem="false" enable_currentUserHome="true" enable_anywhere="false"'
else
    DOMAIN_ATTR='enable_localSystem="true" enable_currentUserHome="false" enable_anywhere="false"'
fi

cat > "$STAGE_DIR/distribution.xml" << DIST_EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>QUASAR ${VERSION}</title>
    <organization>com.patchwork</organization>
    <domains ${DOMAIN_ATTR}/>
    <options customize="never" require-scripts="false" rootVolumeOnly="true"/>
    <welcome file="welcome.txt" mime-type="text/plain"/>
    <license file="LICENSE.txt" mime-type="text/plain"/>
    <conclusion file="conclusion.txt" mime-type="text/plain"/>
    <choices-outline>
        <line choice="quasar"/>
    </choices-outline>
    <choice id="quasar" title="QUASAR" description="Binaural spatial AU + factory presets">
        <pkg-ref id="com.patchwork.quasar.${SCOPE_SUFFIX}"/>
    </choice>
    <pkg-ref id="com.patchwork.quasar.${SCOPE_SUFFIX}" version="${VERSION}">quasar.pkg</pkg-ref>
</installer-gui-script>
DIST_EOF

mkdir -p "$DIST_DIR"
PKG_PATH="$DIST_DIR/$PKG_NAME"

echo "==> Building installer package..."
productbuild --distribution "$STAGE_DIR/distribution.xml" --resources "$RES_DIR" --package-path "$STAGE_DIR/components" "$PKG_PATH"

cat > "$DIST_DIR/quasar-version.json" << VERSION_EOF
{
  "name": "QUASAR",
  "version": "${VERSION}",
  "released": "$(date -u +%Y-%m-%d)",
  "platform": "macos",
  "arch": "arm64",
  "install_target": "${TARGET}",
  "pkg": "${PKG_NAME}",
  "min_macos": "13.0",
  "release_notes_url": "https://github.com/cbackstrom80/patchwork-eight/releases/tag/quasar-v${VERSION}"
}
VERSION_EOF

if [[ "$MAKE_DMG" -eq 1 ]]; then
    DMG_STAGE="$STAGE_DIR/dmg"
    rm -rf "$DMG_STAGE"
    mkdir -p "$DMG_STAGE"
    cp "$PKG_PATH" "$DMG_STAGE/"
    cp "$REPO_ROOT/docs/QUASAR_STANDALONE_PLUGIN.md" "$DMG_STAGE/QUASAR — Read Me.txt"
    cp "$REPO_ROOT/docs/INSTALL.md" "$DMG_STAGE/INSTALL.txt"
    ln -s /Applications "$DMG_STAGE/Applications" 2>/dev/null || true
    DMG_PATH="$DIST_DIR/QUASAR-${VERSION}-macOS-${SCOPE_SUFFIX}.dmg"
    hdiutil create -volname "QUASAR ${VERSION}" -srcfolder "$DMG_STAGE" -ov -format UDZO "$DMG_PATH" >/dev/null
    echo "==> DMG: $DMG_PATH"
fi

echo ""
echo "==> Done."
echo "    Package: $PKG_PATH ($(du -h "$PKG_PATH" | cut -f1))"
echo "    Version: $DIST_DIR/quasar-version.json"
if [[ "$TARGET" == "user" ]]; then
    echo ""
    echo "Install (no admin password):"
    echo "  open \"$PKG_PATH\""
fi
