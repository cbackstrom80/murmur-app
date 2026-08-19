#!/usr/bin/env bash
# Shared helpers for MURMUR local deploy / cleanup scripts.
# Source from other scripts; do not execute directly.

murmur_repo_root() {
  if [[ -n "${REPO_ROOT:-}" ]]; then
    echo "$REPO_ROOT"
    return
  fi
  local here
  here="$(cd "$(dirname "${BASH_SOURCE[1]}")/../.." && pwd)"
  echo "$here"
}

murmur_bundle_version() {
  local plist="$1"
  if [[ ! -f "$plist" ]]; then
    echo "?"
    return
  fi
  local ver build
  ver="$(plutil -extract CFBundleShortVersionString raw "$plist" 2>/dev/null || echo '?')"
  build="$(plutil -extract CFBundleVersion raw "$plist" 2>/dev/null || echo '?')"
  echo "${ver} (${build})"
}

murmur_kill_running() {
  echo "==> Stop running MURMUR processes"
  killall MURMUR 2>/dev/null || true
  sleep 0.5
}

murmur_clear_au_cache() {
  echo "==> Clear AU cache + restart AudioComponentRegistrar"
  rm -rf "${HOME}/Library/Caches/AudioUnitCache/com.apple.audiounits.cache" 2>/dev/null || true
  killall -9 AudioComponentRegistrar 2>/dev/null || true
}

murmur_cleanup_installs() {
  local dry_run="${1:-0}"
  local remove_app="${HOME}/Applications/MURMUR.app"
  local remove_au="${HOME}/Library/Audio/Plug-Ins/Components/MURMUR.component"
  local remove_vst3="${HOME}/Library/Audio/Plug-Ins/VST3/MURMUR.vst3"

  echo "==> Remove old MURMUR user installs"

  for target in "$remove_app" "$remove_au" "$remove_vst3"; do
    if [[ -e "$target" ]]; then
      local label ver
      if [[ -d "$target" && -f "${target}/Contents/Info.plist" ]]; then
        ver="$(murmur_bundle_version "${target}/Contents/Info.plist")"
        label="$(basename "$target") ${ver}"
      else
        label="$(basename "$target")"
      fi
      if [[ "$dry_run" == "1" ]]; then
        echo "    [dry-run] rm -rf \"$target\"  ($label)"
      else
        echo "    removing: $label"
        rm -rf "$target"
      fi
    fi
  done

  if [[ "$dry_run" == "1" ]]; then
    echo "    [dry-run] clear AU cache"
  else
    murmur_clear_au_cache
  fi
}

murmur_newest_artefact() {
  local kind="$1"
  local config="${2:-Release}"
  local repo="$3"
  local suffix
  case "$kind" in
    au) suffix="AU/MURMUR.component" ;;
    vst3) suffix="VST3/MURMUR.vst3" ;;
    standalone) suffix="Standalone/MURMUR.app" ;;
    *)
      echo "unknown artefact kind: $kind" >&2
      return 1
      ;;
  esac

  local candidates=(
    "${repo}/build/pw8_plugin_artefacts/${config}/${suffix}"
    "${repo}/build/plugin/pw8_plugin_artefacts/${config}/${suffix}"
    "${repo}/build/plugin-release/plugin/pw8_plugin_artefacts/${config}/${suffix}"
    "${repo}/build/plugin/plugin/pw8_plugin_artefacts/Debug/${suffix/AU\/MURMUR.component/AU/MURMUR.component}"
    "${repo}/build/dev/plugin/pw8_plugin_artefacts/Debug/${suffix}"
  )

  local best="" best_mtime=0 candidate binary mtime
  for candidate in "${candidates[@]}"; do
    [[ -d "$candidate" ]] || continue
    if [[ "$kind" == "standalone" ]]; then
      binary="${candidate}/Contents/MacOS/MURMUR"
    else
      binary="${candidate}/Contents/MacOS/MURMUR"
    fi
    [[ -f "$binary" ]] || continue
    mtime=$(stat -f '%m' "$binary" 2>/dev/null || stat -c '%Y' "$binary" 2>/dev/null || echo 0)
    if [[ "$mtime" -gt "$best_mtime" ]]; then
      best="$candidate"
      best_mtime="$mtime"
    fi
  done

  if [[ -z "$best" ]]; then
    return 1
  fi
  echo "$best"
}

murmur_cleanup_build_artifacts() {
  local repo="$1"
  local keep_preset="${2:-plugin-release}"
  local dry_run="${3:-0}"
  local keep_pkg_stage="${4:-0}"

  local keep_dir="${repo}/build/${keep_preset}"
  local removed_bytes=0

  murmur_remove_path() {
    local path="$1"
    local reason="$2"
    [[ -e "$path" ]] || return 0
    local size
    size=$(du -sk "$path" 2>/dev/null | awk '{print $1}')
    if [[ "$dry_run" == "1" ]]; then
      echo "    [dry-run] rm -rf \"$path\"  ($reason, ~$(( size / 1024 ))M)"
    else
      echo "    removing: $path  ($reason, ~$(( size / 1024 ))M)"
      rm -rf "$path"
    fi
    removed_bytes=$((removed_bytes + size))
  }

  echo "==> Prune stale build trees (keeping build/${keep_preset})"

  case "$keep_preset" in
    plugin-release)
      murmur_remove_path "${repo}/build/plugin" "superseded Debug plugin tree"
      ;;
    plugin)
      murmur_remove_path "${repo}/build/plugin-release" "superseded Release plugin tree"
      ;;
  esac

  if [[ "$keep_pkg_stage" != "1" ]]; then
    murmur_remove_path "${repo}/build/plugin-release/pkg-stage" "old pkg staging copy"
    murmur_remove_path "${repo}/build/plugin/pkg-stage" "old pkg staging copy"
  fi

  murmur_remove_path "${repo}/build/quasar_plugin" "legacy standalone QUASAR scaffold"
  murmur_remove_path "${repo}/build/dev/plugin" "stale dev plugin artefacts"

  if [[ "$dry_run" == "1" ]]; then
    echo "    [dry-run] done (~$(( removed_bytes / 1024 ))M would be reclaimed)"
  else
    echo "    reclaimed ~$(( removed_bytes / 1024 ))M"
  fi
}
