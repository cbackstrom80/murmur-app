#!/usr/bin/env bash
# scripts/build_release_pkg.sh — build MURMUR macOS release artifacts (.pkg, optional .dmg)
#
# Installs:
#   AU  -> ~/Library or /Library/Audio/Plug-Ins/Components/MURMUR.component
#   VST3, Standalone, factory presets, wavetables (optional components)
#
# Usage:
#   scripts/build_release_pkg.sh [options] [version]
#
# Options:
#   --target user|system     Install scope (default: user — no admin password)
#   --dmg                    Also create a drag-and-install .dmg wrapper
#   --full                   Include VST3 + Standalone (default is AU + content only)
#   --system-only            Alias for default AU-only scope (kept for CI/scripts)
#   --sign IDENTITY          Code-sign bundles (Developer ID Application: …)
#   --notarize               Notarize the .pkg (requires --sign and Apple credentials)
#   --skip-auval             Skip hard-fail auval gate (not recommended for shipping)
#   -h, --help               Show help
#
# Environment (optional, for --notarize):
#   APPLE_ID, APPLE_APP_SPECIFIC_PASSWORD, APPLE_TEAM_ID
#   CODESIGN_IDENTITY          Same as --sign if flag omitted
#
# Examples:
#   scripts/build_release_pkg.sh                     # Ben-friendly user pkg
#   scripts/build_release_pkg.sh --target system 1.0.0
#   scripts/build_release_pkg.sh --dmg --target user 0.2.0
#   CODESIGN_IDENTITY="Developer ID Application: …" scripts/build_release_pkg.sh --target system --sign "$CODESIGN_IDENTITY" --notarize 1.0.0

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

TARGET="user"
MAKE_DMG=0
# Ben-friendly default: AU + presets only (no VST3/Standalone).
SYSTEM_ONLY=1
SIGN_IDENTITY="${CODESIGN_IDENTITY:-}"
DO_NOTARIZE=0
SKIP_AUVAL=0
VERSION=""

usage() {
    sed -n '2,28p' "$0" | sed 's/^# \?//'
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
        --full)
            SYSTEM_ONLY=0
            shift
            ;;
        --system-only)
            SYSTEM_ONLY=1
            shift
            ;;
        --sign)
            SIGN_IDENTITY="${2:?--sign requires an identity string}"
            shift 2
            ;;
        --notarize)
            DO_NOTARIZE=1
            shift
            ;;
        --skip-auval)
            SKIP_AUVAL=1
            shift
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

resolve_release_notes() {
    local ver="$1"
    local base="${ver%%-*}"
    for candidate in \
        "$REPO_ROOT/docs/RELEASE_${ver}.md" \
        "$REPO_ROOT/docs/RELEASE_${base}.md"
    do
        if [[ -f "$candidate" ]]; then
            echo "$candidate"
            return 0
        fi
    done
    return 1
}

if [[ -z "$VERSION" ]]; then
    VERSION="$(python3 - <<'PY'
import re, pathlib
text = pathlib.Path("CMakeLists.txt").read_text()
m = re.search(r'project\s*\(\s*patchwork_eight\s*\n\s*VERSION\s+([0-9.]+)', text, re.M)
print(m.group(1) if m else "0.1.0")
PY
)-rc.$(date +%Y%m%d)"
fi

RELEASE_NOTES="$(resolve_release_notes "$VERSION" || true)"

BUILD_DIR="build/plugin-release"
ARTEFACT_DIR="$BUILD_DIR/plugin/pw8_plugin_artefacts/Release"
STAGE_DIR="$BUILD_DIR/pkg-stage"
DIST_DIR="dist"
if [[ "$TARGET" == "user" && "$SYSTEM_ONLY" -eq 1 ]]; then
    SCOPE_SUFFIX="arm64"
elif [[ "$TARGET" == "user" && "$SYSTEM_ONLY" -eq 0 ]]; then
    SCOPE_SUFFIX="arm64-full"
else
    SCOPE_SUFFIX="${TARGET}$([[ "$SYSTEM_ONLY" -eq 1 ]] && echo '-au' || echo '-full')"
fi
PKG_NAME="MURMUR-${VERSION}-macOS-${SCOPE_SUFFIX}.pkg"

VST3_SRC="$ARTEFACT_DIR/VST3/MURMUR.vst3"
AU_SRC="$ARTEFACT_DIR/AU/MURMUR.component"
APP_SRC="$ARTEFACT_DIR/Standalone/MURMUR.app"
PRESETS_SRC="content/presets/factory"
SHOWCASE_PRESETS_SRC="content/presets"
WAVETABLES_SRC="content/wavetables"
DESIGN_FX_SRC="content/design-fx"

if [[ "$TARGET" == "user" ]]; then
    INSTALL_BLURB="your home folder (no admin password):
  ~/Library/Audio/Plug-Ins/Components/MURMUR.component
  ~/Library/Application Support/MURMUR/Presets/"
    PRESET_INSTALL_PATH='~/Library/Application Support/MURMUR/Presets/factory'
else
    INSTALL_BLURB="system-wide locations (admin password required):
  /Library/Audio/Plug-Ins/Components/MURMUR.component
  /Library/Application Support/MURMUR/Presets/"
    PRESET_INSTALL_PATH='/Library/Application Support/MURMUR/Presets/factory'
fi

echo "==> MURMUR macOS release builder"
echo "    version: $VERSION"
echo "    target:  $TARGET ($SCOPE_SUFFIX pkg)"
echo "    arch:    $(uname -m) native (Apple Silicon arm64 on M-series Macs)"

if [[ ! -d "$AU_SRC" ]] || { [[ "$SYSTEM_ONLY" -eq 0 ]] && [[ ! -d "$VST3_SRC" || ! -d "$APP_SRC" ]]; }; then
    echo "==> Release artifacts not found — building via plugin-release preset (arm64)..."
    cmake --preset plugin-release -DCMAKE_OSX_ARCHITECTURES=arm64
    if [[ "$SYSTEM_ONLY" -eq 1 ]]; then
        cmake --build --preset plugin-release -j --target pw8_plugin_AU
    else
        cmake --build --preset plugin-release -j
    fi
fi

for artifact in "$AU_SRC"; do
    if [[ ! -d "$artifact" ]]; then
        echo "ERROR: expected build artifact missing: $artifact" >&2
        exit 1
    fi
done

if [[ "$SYSTEM_ONLY" -eq 0 ]]; then
    for artifact in "$VST3_SRC" "$APP_SRC"; do
        if [[ ! -d "$artifact" ]]; then
            echo "ERROR: expected build artifact missing: $artifact" >&2
            exit 1
        fi
    done
fi

if [[ ! -d "$PRESETS_SRC" ]] || [[ -z "$(find "$PRESETS_SRC" -name '*.pw8' -print -quit 2>/dev/null)" ]]; then
    echo "==> Factory presets not found — generating..."
    python3 scripts/generate_factory_presets.py
fi
PRESET_COUNT=$(find "$PRESETS_SRC" -name '*.pw8' | wc -l | tr -d ' ')
INTERSTELLAR_COUNT=$(find "$PRESETS_SRC/Interstellar" -name '*.pw8' 2>/dev/null | wc -l | tr -d ' ')
echo "==> Bundling $PRESET_COUNT factory presets (${INTERSTELLAR_COUNT} Interstellar)."
if [[ ! -d "$PRESETS_SRC/Interstellar" ]] || [[ "$INTERSTELLAR_COUNT" -lt 100 ]]; then
    echo "ERROR: Interstellar factory bank missing or incomplete under $PRESETS_SRC/Interstellar" >&2
    echo "       Run: python3 scripts/generate_interstellar_presets.py" >&2
    exit 1
fi

if [[ ! -d "$WAVETABLES_SRC" ]] || [[ -z "$(find "$WAVETABLES_SRC" -name '*.json' -print -quit 2>/dev/null)" ]]; then
    echo "ERROR: wavetable library missing under $WAVETABLES_SRC" >&2
    echo "       Run: python3 scripts/generate_wavetable_library.py" >&2
    exit 1
fi
WAVETABLE_COUNT=$(find "$WAVETABLES_SRC" -maxdepth 1 -name '*.json' | wc -l | tr -d ' ')
echo "==> Bundling $WAVETABLE_COUNT wavetable JSON files."

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
if [[ "$SYSTEM_ONLY" -eq 0 && -f "${APP_SRC}/Contents/Info.plist" ]]; then
    # Distinct standalone ID prevents PackageKit from relocating the AU into MURMUR.app.
    plutil -replace CFBundleIdentifier -string "com.patchwork.murmur.standalone" "${APP_SRC}/Contents/Info.plist"
fi
sign_bundle "$AU_SRC"
if [[ "$SYSTEM_ONLY" -eq 0 ]]; then
    sign_bundle "$VST3_SRC"
    sign_bundle "$APP_SRC"
fi

echo "==> Staging install payloads ($TARGET)..."
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/components"

make_root() {
    local name="$1"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Audio/Plug-Ins/Components"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Audio/Plug-Ins/VST3"
    mkdir -p "$STAGE_DIR/${name}-root/Applications"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Application Support/MURMUR/Presets/factory"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Application Support/MURMUR/Presets/.murmur-factory-staging"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Application Support/MURMUR/Presets/showcase"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Application Support/MURMUR/Wavetables"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Application Support/MURMUR/design-fx"
    mkdir -p "$STAGE_DIR/${name}-root/Library/Application Support/MURMUR/design-fx/user"
}

make_root "payload"
PAYLOAD="$STAGE_DIR/payload-root"

cp -R "$AU_SRC" "$PAYLOAD/Library/Audio/Plug-Ins/Components/"
if [[ "$SYSTEM_ONLY" -eq 0 ]]; then
    cp -R "$VST3_SRC" "$PAYLOAD/Library/Audio/Plug-Ins/VST3/"
    cp -R "$APP_SRC" "$PAYLOAD/Applications/"
fi
FACTORY_DST="$PAYLOAD/Library/Application Support/MURMUR/Presets/factory"
STAGING_DST="$PAYLOAD/Library/Application Support/MURMUR/Presets/.murmur-factory-staging"
cp -R "$PRESETS_SRC/"* "$FACTORY_DST/"
cp -R "$PRESETS_SRC/"* "$STAGING_DST/"

STAGED_INTERSTELLAR=$(find "$FACTORY_DST/Interstellar" -name '*.pw8' 2>/dev/null | wc -l | tr -d ' ')
if [[ "$STAGED_INTERSTELLAR" -lt 100 ]]; then
    echo "ERROR: pkg payload missing Interstellar bank (found ${STAGED_INTERSTELLAR} .pw8 under factory/Interstellar)" >&2
    exit 1
fi
echo "==> Staged ${PRESET_COUNT} factory presets in pkg payload (${STAGED_INTERSTELLAR} Interstellar)."

find "$SHOWCASE_PRESETS_SRC" -maxdepth 1 -name '*.pw8' -exec cp {} \
    "$PAYLOAD/Library/Application Support/MURMUR/Presets/showcase/" \;
cp "$WAVETABLES_SRC"/*.json "$PAYLOAD/Library/Application Support/MURMUR/Wavetables/"

if [[ -d "$DESIGN_FX_SRC" ]]; then
    cp -R "$DESIGN_FX_SRC/"* "$PAYLOAD/Library/Application Support/MURMUR/design-fx/" 2>/dev/null || true
    mkdir -p "$PAYLOAD/Library/Application Support/MURMUR/design-fx/user"
    DESIGN_FX_COUNT=$(find "$PAYLOAD/Library/Application Support/MURMUR/design-fx" -maxdepth 1 -name '*.json' | wc -l | tr -d ' ')
    echo "==> Staged ${DESIGN_FX_COUNT} design-FX preset files."
fi

# Logic / Ben MVP docs shipped beside presets.
DOCS_DIR="$PAYLOAD/Library/Application Support/MURMUR/Docs"
mkdir -p "$DOCS_DIR"
for doc in BEN_DEMO_PRESETS.md BEN_MVP.md DESIGN_AND_WARPS_PLAN.md INSTALL.md KAWAI_MP11SE.md LOGIC_SMART_CONTROLS.md MIDI_CONTROLLERS.md PRODUCT_GAP_PLAN.md; do
    if [[ -f "$REPO_ROOT/docs/$doc" ]]; then
        cp "$REPO_ROOT/docs/$doc" "$DOCS_DIR/"
    fi
done
if [[ -n "$RELEASE_NOTES" && -f "$RELEASE_NOTES" ]]; then
    cp "$RELEASE_NOTES" "$DOCS_DIR/WHATS_NEW.md"
fi
if [[ -d "$REPO_ROOT/docs/product" ]]; then
    mkdir -p "$DOCS_DIR/product"
    cp "$REPO_ROOT/docs/product/"*.md "$DOCS_DIR/product/"
fi

# README dropped next to presets for first-run guidance.
mkdir -p "$PAYLOAD/Library/Application Support/MURMUR"
cat > "$PAYLOAD/Library/Application Support/MURMUR/README.txt" << README_EOF
MURMUR ${VERSION} — Ben MVP
============================

Logic Pro (Apple Silicon): AU Instruments → Murmur → MURMUR

Factory presets (${PRESET_COUNT}):
  ${PRESET_INSTALL_PATH}

Logic + Kawai MP11SE setup:
  ~/Library/Application Support/MURMUR/Docs/KAWAI_MP11SE.md
  ~/Library/Application Support/MURMUR/Docs/LOGIC_SMART_CONTROLS.md

Product documentation (start here):
  ~/Library/Application Support/MURMUR/Docs/product/README.md

After installing: quit Logic, reopen, Plug-in Manager → Reset & Rescan.

Updates: https://github.com/cbackstrom80/patchwork-eight/releases
README_EOF

PKG_SCRIPTS="$STAGE_DIR/pkg-scripts"
mkdir -p "$PKG_SCRIPTS"
cp "$REPO_ROOT/scripts/pkg/postinstall" "$PKG_SCRIPTS/postinstall"
chmod +x "$PKG_SCRIPTS/postinstall"

pkgbuild --root "$PAYLOAD" \
    --identifier "com.patchwork.murmur.${SCOPE_SUFFIX}" \
    --version "$VERSION" \
    --install-location "/" \
    --scripts "$PKG_SCRIPTS" \
    "$STAGE_DIR/components/murmur.pkg" >/dev/null

RES_DIR="$STAGE_DIR/resources"
mkdir -p "$RES_DIR"

OPTIONAL_FORMATS_LINE=""
if [[ "$SYSTEM_ONLY" -eq 0 ]]; then
    OPTIONAL_FORMATS_LINE="  • VST3 plug-in and Standalone app"
fi

if [[ -n "$SIGN_IDENTITY" ]]; then
    SIGNING_BLURB="Developer ID signed"
else
    SIGNING_BLURB="ad-hoc signed (see docs/INSTALL.md for Gatekeeper)"
fi

cat > "$RES_DIR/welcome.txt" << WELCOME_EOF
MURMUR ${VERSION} — Ben MVP
8-Engine Algorithmic Synthesizer for Logic Pro

This installer is built for Apple Silicon Macs and Logic Pro.
It places MURMUR in ${INSTALL_BLURB}

Included:
  • MURMUR Audio Unit (Logic: AU Instruments → Murmur → MURMUR)
${OPTIONAL_FORMATS_LINE}
  • ${PRESET_COUNT} factory presets (Basses, Leads, Pads, Sequences, Ambient, Interstellar)
  • ${WAVETABLE_COUNT} wavetable files
  • Design FX preset library (Application Support/MURMUR/design-fx/)
  • Logic + Kawai MP11SE setup guides (Application Support/MURMUR/Docs/)
  • MURMUR product documentation (Docs/product/)

After installing:
  1. Quit Logic completely and reopen
  2. Plug-in Manager → Reset & Rescan Selection
  3. Add MURMUR on a Software Instrument track
  4. BROWSE or LOAD... to pick a factory preset

Signing: ${SIGNING_BLURB}
WELCOME_EOF

cat > "$RES_DIR/conclusion.txt" << CONCLUSION_EOF
MURMUR is installed — ready for Logic Pro.

NEXT STEPS
----------
1. Quit and reopen Logic Pro.
2. Settings → Plug-in Manager → Reset & Rescan Selection.
3. New track → Software Instrument → AU Instruments → Murmur → MURMUR.
4. In MURMUR PLAY mode: BROWSE or LOAD... for factory presets.

KAWAI MP11SE (optional)
-----------------------
Open the setup guide (Finder → Go → Go to Folder):
  ~/Library/Application Support/MURMUR/Docs/KAWAI_MP11SE.md

Presets folder:
  ${PRESET_INSTALL_PATH}

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
    <title>MURMUR ${VERSION}</title>
    <organization>com.patchwork</organization>
    <domains ${DOMAIN_ATTR}/>
    <options customize="never" require-scripts="false" rootVolumeOnly="true"/>
    <welcome file="welcome.txt" mime-type="text/plain"/>
    <license file="LICENSE.txt" mime-type="text/plain"/>
    <conclusion file="conclusion.txt" mime-type="text/plain"/>
    <choices-outline>
        <line choice="murmur"/>
    </choices-outline>
    <choice id="murmur" title="MURMUR" description="Plug-in, presets, and wavetables">
        <pkg-ref id="com.patchwork.murmur.${SCOPE_SUFFIX}"/>
    </choice>
    <pkg-ref id="com.patchwork.murmur.${SCOPE_SUFFIX}" version="${VERSION}">murmur.pkg</pkg-ref>
</installer-gui-script>
DIST_EOF

mkdir -p "$DIST_DIR"
PKG_PATH="$DIST_DIR/$PKG_NAME"
DMG_PATH=""

echo "==> Building installer package..."
PRODUCTBUILD_ARGS=(--distribution "$STAGE_DIR/distribution.xml" --resources "$RES_DIR" --package-path "$STAGE_DIR/components")
if [[ -n "$SIGN_IDENTITY" ]]; then
    INSTALLER_ID="${CODESIGN_INSTALLER_IDENTITY:-${SIGN_IDENTITY/Application/Installer}}"
    if security find-identity -v -p codesigning 2>/dev/null | grep -q "Developer ID Installer"; then
        PRODUCTBUILD_ARGS+=(--sign "$INSTALLER_ID")
    fi
fi
productbuild "${PRODUCTBUILD_ARGS[@]}" "$PKG_PATH"

if [[ "$DO_NOTARIZE" -eq 1 ]]; then
    if [[ -z "$SIGN_IDENTITY" ]]; then
        echo "ERROR: --notarize requires --sign (Developer ID)" >&2
        exit 1
    fi
    : "${APPLE_ID:?Set APPLE_ID for notarization}"
    : "${APPLE_APP_SPECIFIC_PASSWORD:?Set APPLE_APP_SPECIFIC_PASSWORD}"
    : "${APPLE_TEAM_ID:?Set APPLE_TEAM_ID}"
    echo "==> Notarizing (stapling)..."
    xcrun notarytool submit "$PKG_PATH" --apple-id "$APPLE_ID" --password "$APPLE_APP_SPECIFIC_PASSWORD" --team-id "$APPLE_TEAM_ID" --wait
    xcrun stapler staple "$PKG_PATH"
fi

# version.json for update checks / release automation
mkdir -p "$DIST_DIR"
cat > "$DIST_DIR/version.json" << VERSION_EOF
{
  "name": "MURMUR",
  "version": "${VERSION}",
  "released": "$(date -u +%Y-%m-%d)",
  "platform": "macos",
  "arch": "arm64",
  "install_target": "${TARGET}",
  "pkg": "${PKG_NAME}",
  "min_macos": "13.0",
  "release_notes_url": "https://github.com/cbackstrom80/patchwork-eight/releases/tag/v${VERSION%%-*}"
}
VERSION_EOF

if [[ "$MAKE_DMG" -eq 1 ]]; then
    DMG_STAGE="$STAGE_DIR/dmg"
    rm -rf "$DMG_STAGE"
    mkdir -p "$DMG_STAGE"
    cp "$PKG_PATH" "$DMG_STAGE/Install MURMUR.pkg"
    cp "$REPO_ROOT/docs/BEN_MVP.md" "$DMG_STAGE/1 — READ ME FIRST.txt"
    cp "$REPO_ROOT/docs/product/README.md" "$DMG_STAGE/MURMUR Product Docs.txt"
    cp "$REPO_ROOT/docs/INSTALL.md" "$DMG_STAGE/INSTALL.txt"
    if [[ -n "$RELEASE_NOTES" && -f "$RELEASE_NOTES" ]]; then
        cp "$RELEASE_NOTES" "$DMG_STAGE/WHATS NEW.txt"
    elif [[ -f "$REPO_ROOT/docs/RELEASE_1.4.0.md" ]]; then
        cp "$REPO_ROOT/docs/RELEASE_1.4.0.md" "$DMG_STAGE/WHATS NEW.txt"
    fi
    cp "$REPO_ROOT/docs/LOGIC_SMART_CONTROLS.md" "$DMG_STAGE/LOGIC SETUP.txt" 2>/dev/null || true
    if [[ "$SYSTEM_ONLY" -eq 0 && -d "$APP_SRC" ]]; then
        cp -R "$APP_SRC" "$DMG_STAGE/MURMUR.app"
        ln -s /Applications "$DMG_STAGE/Applications" 2>/dev/null || true
    fi
    cat > "$DMG_STAGE/2 — Double-click Install MURMUR.pkg.txt" << 'DMG_README'
MURMUR — Installation (Apple Silicon Mac)
=========================================

1. Double-click "Install MURMUR.pkg"
2. Follow the installer (no admin password — installs to your home folder)
3. Quit Logic Pro completely, then reopen
4. Logic → Settings → Plug-in Manager → Reset & Rescan Selection
5. New Software Instrument track → AU Instruments → Murmur → MURMUR

Optional: drag MURMUR.app to Applications to launch standalone (test without Logic).

If macOS blocks the installer: right-click Install MURMUR.pkg → Open → Open.

Presets: click the preset name in the header to open Preset Explorer.

Support docs are in "1 — READ ME FIRST.txt" and INSTALL.txt.
DMG_README
    if [[ "$SYSTEM_ONLY" -eq 1 ]]; then
        PKG_SUFFIX="arm64"
    else
        PKG_SUFFIX="${SCOPE_SUFFIX}"
    fi
    DMG_PATH="$DIST_DIR/MURMUR-${VERSION}-macOS-${PKG_SUFFIX}.dmg"
    # Use /tmp for hdiutil scratch — avoids failures when repo volume is nearly full
    if ! TMPDIR="${TMPDIR:-/tmp}" hdiutil create -volname "MURMUR ${VERSION}" -srcfolder "$DMG_STAGE" -ov -format UDZO "$DMG_PATH" >/dev/null; then
        echo "ERROR: DMG creation failed (often disk space). pkg is still at: $PKG_PATH" >&2
        exit 1
    fi
    echo "==> DMG: $DMG_PATH"
fi

echo ""
echo "==> Verifying AU (auval)..."
if [[ "$SKIP_AUVAL" -eq 1 ]]; then
    echo "    (skipped — --skip-auval)"
elif command -v auval >/dev/null 2>&1; then
    mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
    rsync -a --delete "$AU_SRC/" "$HOME/Library/Audio/Plug-Ins/Components/MURMUR.component/"
    rm -rf "$HOME/Library/Caches/AudioUnitCache/com.apple.audiounits.cache" 2>/dev/null || true
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    for _ in $(seq 1 10); do
        if auval -v aumu Murm Murr >/dev/null 2>&1; then
            break
        fi
        sleep 1
    done
    if auval -v aumu Murm Murr; then
        echo "==> auval: PASS"
    else
        echo "ERROR: auval FAILED — fix before shipping (or pass --skip-auval for dev only)" >&2
        exit 1
    fi
else
    echo "ERROR: auval not found — install Xcode CLT" >&2
    exit 1
fi

if [[ -f "$AU_SRC/Contents/MacOS/MURMUR" ]]; then
    echo "==> Binary arch: $(lipo -info "$AU_SRC/Contents/MacOS/MURMUR" 2>/dev/null || file "$AU_SRC/Contents/MacOS/MURMUR")"
    echo "==> Bundle version: $(plutil -extract CFBundleShortVersionString raw "$AU_SRC/Contents/Info.plist" 2>/dev/null || echo 'unknown')"
    echo "==> Min macOS: $(plutil -extract LSMinimumSystemVersion raw "$AU_SRC/Contents/Info.plist" 2>/dev/null || echo 'unknown')"
fi

# Checksums for Ben / release integrity
CHECKSUM_FILE="$DIST_DIR/SHA256SUMS.txt"
{
    echo "# MURMUR ${VERSION} — $(date -u +%Y-%m-%dT%H:%MZ)"
    (cd "$DIST_DIR" && shasum -a 256 "$(basename "$PKG_PATH")")
    [[ -f "${DMG_PATH:-}" ]] && (cd "$DIST_DIR" && shasum -a 256 "$(basename "$DMG_PATH")")
} > "$CHECKSUM_FILE"
echo "==> Checksums: $CHECKSUM_FILE"

echo ""
echo "==> Done."
echo "    Package: $PKG_PATH ($(du -h "$PKG_PATH" | cut -f1))"
echo "    Version: $DIST_DIR/version.json"
if [[ "$TARGET" == "user" ]]; then
    echo ""
    echo "Install (Ben — no admin password):"
    echo "  open \"$PKG_PATH\""
else
    echo ""
    echo "Install (system-wide, admin password):"
    echo "  sudo installer -pkg \"$PKG_PATH\" -target /"
fi
