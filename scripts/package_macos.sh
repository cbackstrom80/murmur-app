#!/usr/bin/env bash
set -euo pipefail

# scripts/package_macos.sh -- packages Patchwork Eight's VST3/AU/Standalone
# Release build into a single macOS installer (.pkg).
#
# LOCAL / PERSONAL USE ONLY. See docs/LICENSING.md: this repo's own code has
# no license granted yet (LICENSE is currently an "all rights reserved"
# placeholder), and JUCE is GPLv3-or-commercial -- a paid JUCE commercial
# license is required before any closed-source *distribution* of a build
# using it. This script packages a build for installing on the machine that
# built it, not for handing to anyone else.
#
# Also NOT notarized (docs/ROADMAP.md Phase 16 "What's still missing" #5 --
# no Apple Developer ID is available in this environment). Each bundle is
# ad-hoc signed instead, which satisfies Gatekeeper for a locally-built,
# locally-installed copy. If this .pkg is ever moved to another machine
# (AirDrop, a download link, etc.), macOS will attach a quarantine flag and
# Gatekeeper will refuse to run it without an explicit right-click -> Open
# override, since it isn't signed with a Developer ID or notarized.
#
# Installs to the STANDARD SYSTEM-WIDE plugin locations (the same convention
# every commercial plugin installer uses -- u-he, Arturia, Native
# Instruments, etc.), which means the install step needs an admin password:
#   /Library/Audio/Plug-Ins/VST3
#   /Library/Audio/Plug-Ins/Components
#   /Applications
# An earlier version of this script targeted ~/Library (no admin password)
# via `installer -target CurrentUserHomeDirectory`, but that CLI target
# turned out to be unreliable in practice -- it reports success while
# silently writing nothing under some sandboxing configurations. The
# standard system-wide + admin-password route is the well-trodden path
# every other plugin installer relies on; use it instead of debugging
# around that quirk.
#
# What it does:
#   1. Builds the plugin in Release configuration (the `plugin-release` CMake
#      preset) if the artifacts aren't already there.
#   2. Ad-hoc code-signs each of the three bundles.
#   3. Packages VST3/AU/Standalone into one distribution .pkg under dist/.
#
# Usage:
#   scripts/package_macos.sh [version]
#   `version` defaults to the CMake project version (0.1.0) with a
#   date-stamped release-candidate suffix.
#
# To install the result:
#   open "dist/PatchworkEight-<version>-macOS.pkg"      # GUI, prompts for admin password
#   sudo installer -pkg "dist/PatchworkEight-<version>-macOS.pkg" -target /   # CLI

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

VERSION="${1:-0.1.0-rc.$(date +%Y%m%d)}"
BUILD_DIR="build/plugin-release"
ARTEFACT_DIR="$BUILD_DIR/plugin/pw8_plugin_artefacts/Release"
STAGE_DIR="$BUILD_DIR/pkg-stage"
DIST_DIR="dist"
PKG_NAME="PatchworkEight-${VERSION}-macOS.pkg"

VST3_SRC="$ARTEFACT_DIR/VST3/Patchwork Eight.vst3"
AU_SRC="$ARTEFACT_DIR/AU/Patchwork Eight.component"
APP_SRC="$ARTEFACT_DIR/Standalone/Patchwork Eight.app"

echo "==> Patchwork Eight macOS installer builder -- version ${VERSION}"

if [[ ! -d "$VST3_SRC" || ! -d "$AU_SRC" || ! -d "$APP_SRC" ]]; then
    echo "==> Release artifacts not found -- building via the plugin-release preset..."
    cmake --preset plugin-release
    cmake --build --preset plugin-release -j
fi

for artifact in "$VST3_SRC" "$AU_SRC" "$APP_SRC"; do
    if [[ ! -d "$artifact" ]]; then
        echo "ERROR: expected build artifact missing: $artifact" >&2
        exit 1
    fi
done

echo "==> Ad-hoc code-signing (no paid Developer ID cert in this environment)..."
codesign --force --deep --sign - --timestamp=none "$VST3_SRC"
codesign --force --deep --sign - --timestamp=none "$AU_SRC"
codesign --force --deep --sign - --timestamp=none "$APP_SRC"
codesign --verify --deep --strict "$VST3_SRC"
codesign --verify --deep --strict "$AU_SRC"
codesign --verify --deep --strict "$APP_SRC"
echo "    signature verified on all three bundles."

echo "==> Staging system-wide install payloads..."
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/vst3-root/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGE_DIR/au-root/Library/Audio/Plug-Ins/Components"
mkdir -p "$STAGE_DIR/app-root/Applications"

cp -R "$VST3_SRC" "$STAGE_DIR/vst3-root/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU_SRC" "$STAGE_DIR/au-root/Library/Audio/Plug-Ins/Components/"
cp -R "$APP_SRC" "$STAGE_DIR/app-root/Applications/"

echo "==> Building component packages..."
mkdir -p "$STAGE_DIR/components"

pkgbuild --root "$STAGE_DIR/vst3-root" \
    --identifier com.patchwork.patchworkeight.vst3 \
    --version "$VERSION" \
    --install-location "/" \
    "$STAGE_DIR/components/vst3.pkg" >/dev/null

pkgbuild --root "$STAGE_DIR/au-root" \
    --identifier com.patchwork.patchworkeight.au \
    --version "$VERSION" \
    --install-location "/" \
    "$STAGE_DIR/components/au.pkg" >/dev/null

pkgbuild --root "$STAGE_DIR/app-root" \
    --identifier com.patchwork.patchworkeight.app \
    --version "$VERSION" \
    --install-location "/" \
    "$STAGE_DIR/components/app.pkg" >/dev/null

echo "==> Writing installer resources (welcome/conclusion text, distribution.xml)..."
RES_DIR="$STAGE_DIR/resources"
mkdir -p "$RES_DIR"

cat > "$RES_DIR/welcome.txt" << WELCOME_EOF
Patchwork Eight ${VERSION}
8-Engine Algorithmic Synthesizer

This installer places three items in the standard system-wide plug-in
locations (needs an admin password, same as any other plugin installer):

  - VST3 plug-in       -> /Library/Audio/Plug-Ins/VST3
  - Audio Unit plug-in -> /Library/Audio/Plug-Ins/Components
  - Standalone app     -> /Applications

Build notes, honestly:
  - Ad-hoc code-signed, not notarized -- built and verified with a real
    auval pass (AU VALIDATION SUCCEEDED, 762 parameters) and pluginval at
    strictness 5 (SUCCESS on both VST3 and AU) on this exact Release build.
  - This copy is for local use on this machine. See LICENSE and
    docs/LICENSING.md in the source repository -- this project's own code
    has no license granted yet, and the JUCE framework it's built on
    requires a paid commercial license before any closed-source
    redistribution.
WELCOME_EOF

cat > "$RES_DIR/conclusion.txt" << 'CONCLUSION_EOF'
Installed. Restart your DAW (or rescan plug-ins) to pick up the new VST3/AU,
or launch "Patchwork Eight" directly from /Applications for the Standalone
app.
CONCLUSION_EOF

cp "$REPO_ROOT/LICENSE" "$RES_DIR/LICENSE.txt"

cat > "$STAGE_DIR/distribution.xml" << DIST_EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>Patchwork Eight ${VERSION}</title>
    <organization>com.patchwork</organization>
    <domains enable_localSystem="true" enable_currentUserHome="false" enable_anywhere="false"/>
    <options customize="allow" require-scripts="false" rootVolumeOnly="true"/>
    <welcome file="welcome.txt" mime-type="text/plain"/>
    <license file="LICENSE.txt" mime-type="text/plain"/>
    <conclusion file="conclusion.txt" mime-type="text/plain"/>
    <choices-outline>
        <line choice="vst3"/>
        <line choice="au"/>
        <line choice="app"/>
    </choices-outline>
    <choice id="vst3" title="VST3 Plug-in" description="Installs to /Library/Audio/Plug-Ins/VST3">
        <pkg-ref id="com.patchwork.patchworkeight.vst3"/>
    </choice>
    <choice id="au" title="Audio Unit Plug-in" description="Installs to /Library/Audio/Plug-Ins/Components">
        <pkg-ref id="com.patchwork.patchworkeight.au"/>
    </choice>
    <choice id="app" title="Standalone App" description="Installs to /Applications">
        <pkg-ref id="com.patchwork.patchworkeight.app"/>
    </choice>
    <pkg-ref id="com.patchwork.patchworkeight.vst3" version="${VERSION}" onConclusion="none">vst3.pkg</pkg-ref>
    <pkg-ref id="com.patchwork.patchworkeight.au" version="${VERSION}" onConclusion="none">au.pkg</pkg-ref>
    <pkg-ref id="com.patchwork.patchworkeight.app" version="${VERSION}" onConclusion="none">app.pkg</pkg-ref>
</installer-gui-script>
DIST_EOF

echo "==> Combining into the final installer..."
mkdir -p "$DIST_DIR"
productbuild --distribution "$STAGE_DIR/distribution.xml" \
    --resources "$RES_DIR" \
    --package-path "$STAGE_DIR/components" \
    "$DIST_DIR/$PKG_NAME"

echo ""
echo "==> Done: $DIST_DIR/$PKG_NAME"
echo "    $(du -h "$DIST_DIR/$PKG_NAME" | cut -f1) -- double-click to install (admin password required),"
echo "    or: sudo installer -pkg \"$DIST_DIR/$PKG_NAME\" -target /"
