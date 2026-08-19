#!/usr/bin/env bash
# scripts/cleanup_murmur_installs.sh — remove old MURMUR.app / AU / VST3 user installs.
#
# Usage:
#   scripts/cleanup_murmur_installs.sh [--dry-run] [--no-kill]
#
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/lib/murmur_deploy_lib.sh
source "${REPO_ROOT}/scripts/lib/murmur_deploy_lib.sh"

DRY_RUN=0
NO_KILL=0
for arg in "$@"; do
  case "$arg" in
    --dry-run) DRY_RUN=1 ;;
    --no-kill) NO_KILL=1 ;;
    -h|--help)
      echo "Usage: scripts/cleanup_murmur_installs.sh [--dry-run] [--no-kill]"
      exit 0
      ;;
    *)
      echo "Unknown option: $arg" >&2
      exit 1
      ;;
  esac
done

cd "$REPO_ROOT"

if [[ "$NO_KILL" != "1" && "$DRY_RUN" != "1" ]]; then
  murmur_kill_running
fi

murmur_cleanup_installs "$DRY_RUN"

echo ""
echo "Done. Reinstall with: scripts/deploy_local.sh"
