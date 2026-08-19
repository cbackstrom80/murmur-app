#!/usr/bin/env bash
# scripts/deploy_local.sh — build MURMUR (AU + VST3 + Standalone) and install locally.
#
# Removes old user installs, optionally prunes superseded build trees, then installs
# fresh artefacts from the configured CMake preset (default: plugin-release).
#
# Usage:
#   scripts/deploy_local.sh [options]
#
# Options:
#   --preset NAME       CMake preset (default: plugin-release)
#   --skip-build        Install newest existing artefacts only
#   --skip-cleanup      Do not remove old ~/Applications / Plug-Ins installs first
#   --skip-prune        Do not delete superseded build/plugin trees after install
#   --no-kill           Do not kill a running MURMUR process before cleanup
#   --dry-run           Print actions without deleting or building
#   -h, --help          Show this help
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/lib/murmur_deploy_lib.sh
source "${REPO_ROOT}/scripts/lib/murmur_deploy_lib.sh"

PRESET="${PW8_DEPLOY_PRESET:-plugin-release}"
SKIP_BUILD=0
SKIP_CLEANUP=0
SKIP_PRUNE=0
NO_KILL=0
DRY_RUN=0

usage() {
  sed -n '2,18p' "$0"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --preset)
      PRESET="$2"
      shift 2
      ;;
    --skip-build) SKIP_BUILD=1; shift ;;
    --skip-cleanup) SKIP_CLEANUP=1; shift ;;
    --skip-prune) SKIP_PRUNE=1; shift ;;
    --no-kill) NO_KILL=1; shift ;;
    --dry-run) DRY_RUN=1; shift ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

cd "$REPO_ROOT"

echo "==> MURMUR local deploy (preset: ${PRESET})"

if [[ "$NO_KILL" != "1" && "$DRY_RUN" != "1" ]]; then
  murmur_kill_running
elif [[ "$DRY_RUN" == "1" ]]; then
  echo "    [dry-run] kill running MURMUR"
fi

if [[ "$SKIP_CLEANUP" != "1" ]]; then
  murmur_cleanup_installs "$DRY_RUN"
else
  echo "==> Skipping install cleanup"
fi

if [[ "$SKIP_BUILD" == "1" ]]; then
  echo "==> Skipping build (install newest existing artefacts)"
else
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "    [dry-run] cmake --preset ${PRESET}"
    echo "    [dry-run] cmake --build --preset ${PRESET} --target pw8_plugin_AU pw8_plugin_VST3 pw8_plugin_Standalone"
  else
    echo "==> Configure (${PRESET})"
    cmake --preset "$PRESET"
    echo "==> Build AU + VST3 + Standalone"
    cmake --build --preset "$PRESET" --target pw8_plugin_AU pw8_plugin_VST3 pw8_plugin_Standalone
  fi
fi

install_one() {
  local script="$1"
  if [[ "$DRY_RUN" == "1" ]]; then
    echo "    [dry-run] ${script}"
  else
    "$script"
  fi
}

echo "==> Install artefacts"
install_one "${REPO_ROOT}/scripts/install_au_local.sh"
install_one "${REPO_ROOT}/scripts/install_vst3_local.sh"
install_one "${REPO_ROOT}/scripts/install_standalone_local.sh"

if [[ "$SKIP_PRUNE" != "1" ]]; then
  murmur_cleanup_build_artifacts "$REPO_ROOT" "$PRESET" "$DRY_RUN" 0
else
  echo "==> Skipping build tree prune"
fi

echo ""
echo "Done. Launch Standalone: open \"${HOME}/Applications/MURMUR.app\""
