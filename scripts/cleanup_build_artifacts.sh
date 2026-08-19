#!/usr/bin/env bash
# scripts/cleanup_build_artifacts.sh — delete superseded plugin build trees to save disk space.
#
# Keeps the active CMake preset directory (default: build/plugin-release) and shared _deps.
#
# Usage:
#   scripts/cleanup_build_artifacts.sh [--keep-preset NAME] [--keep-pkg-stage] [--dry-run]
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/lib/murmur_deploy_lib.sh
source "${REPO_ROOT}/scripts/lib/murmur_deploy_lib.sh"

KEEP_PRESET="${PW8_KEEP_BUILD:-plugin-release}"
KEEP_PKG_STAGE=0
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --keep-preset)
      KEEP_PRESET="$2"
      shift 2
      ;;
    --keep-pkg-stage)
      KEEP_PKG_STAGE=1
      shift
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    -h|--help)
      echo "Usage: scripts/cleanup_build_artifacts.sh [--keep-preset NAME] [--keep-pkg-stage] [--dry-run]"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
  esac
done

cd "$REPO_ROOT"
murmur_cleanup_build_artifacts "$REPO_ROOT" "$KEEP_PRESET" "$DRY_RUN" "$KEEP_PKG_STAGE"
