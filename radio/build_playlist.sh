#!/usr/bin/env bash
# Murmur Radio -- per-station real playlist builder.
#
# Renders a station's patch list through the real DSP engine
# (murmur-render), then crossfades them together into one continuous,
# seamless-loop MP3 with ffmpeg's `acrossfade` filter, plus a real
# manifest.json (pulled from each patch's own metadata -- real track
# names/descriptions, not fabricated copy) for the /radio showcase page.
# The result is what murmur-radio@<station>.service (see radio/README.md)
# loops forever into that station's Icecast mount -- this script is the
# offline "build" step, not something that runs inside the always-on
# service itself (rendering is real DSP work; looping an already-built
# file is cheap).
#
# Station patch lists live in radio/stations/<slug>.txt, one
# `patch:midi:label` line per track (same format the old inline array
# used). Requires murmur-render already built (PW8_BUILD_TOOLS=ON) and
# ffmpeg + python3 on PATH. Real IMPORTANT gotcha this script's callers
# must know: murmur-render resolves every patch's relative paths
# (wavetableId, etc.) against its own working directory, so this script
# always shells out from REPO_ROOT -- same reason mcp_server/render.py
# does the same thing.
#
# Usage: radio/build_playlist.sh <repo_root> <output_dir> <station_slug>
#   e.g. radio/build_playlist.sh /root/murmur-render-check /root/murmur-radio ambient

set -euo pipefail

REPO_ROOT="${1:?Usage: build_playlist.sh <repo_root> <output_dir> <station_slug>}"
OUT_DIR="${2:?Usage: build_playlist.sh <repo_root> <output_dir> <station_slug>}"
STATION="${3:?Usage: build_playlist.sh <repo_root> <output_dir> <station_slug>}"
RENDERER="${REPO_ROOT}/build/render-check/tools/murmur-render"
STATION_LIST="${REPO_ROOT}/radio/stations/${STATION}.txt"
STATION_OUT="${OUT_DIR}/${STATION}"

if [[ ! -x "$RENDERER" ]]; then
  echo "murmur-render not found/executable at $RENDERER -- build it first:" >&2
  echo "  cmake -S . -B build/render-check -G Ninja -DCMAKE_BUILD_TYPE=Release -DPW8_BUILD_TESTS=OFF -DPW8_BUILD_TOOLS=ON" >&2
  echo "  cmake --build build/render-check -j2 --target murmur-render" >&2
  exit 1
fi

if [[ ! -f "$STATION_LIST" ]]; then
  echo "No station list at $STATION_LIST" >&2
  exit 1
fi

mkdir -p "${STATION_OUT}/renders"
cd "$REPO_ROOT"

WAVS=()
PATCH_PATHS=()
while IFS=':' read -r patch midi label; do
  [[ -z "$patch" ]] && continue
  out="${STATION_OUT}/renders/${label}.wav"
  echo "Rendering ${label} (${patch})..."
  "$RENDERER" --patch "$patch" --midi "$midi" --output "$out" --duration 45 --release-tail 3
  WAVS+=("$out")
  PATCH_PATHS+=("$patch")
done < "$STATION_LIST"

# 4-second triangular crossfades, chained pairwise (ffmpeg's acrossfade only
# takes two inputs at a time) -- real overlap, not a hard cut. Built
# dynamically so playlist length isn't hardcoded to any station's size.
echo "Crossfading ${#WAVS[@]} renders into ${STATION_OUT}/playlist.mp3..."

ffmpeg_inputs=()
for w in "${WAVS[@]}"; do
  ffmpeg_inputs+=(-i "$w")
done

filter=""
prev="[0]"
for ((i = 1; i < ${#WAVS[@]}; i++)); do
  next_label="[a$(printf '%02d' "$i")]"
  if (( i == ${#WAVS[@]} - 1 )); then
    next_label="[out]"
  fi
  filter+="${prev}[${i}]acrossfade=d=4:c1=tri:c2=tri${next_label};"
  prev="$next_label"
done
filter="${filter%;}"

ffmpeg -y \
  "${ffmpeg_inputs[@]}" \
  -filter_complex "$filter" \
  -map "[out]" -ar 44100 -c:a libmp3lame -b:a 192k "${STATION_OUT}/playlist.mp3"

echo "Building ${STATION_OUT}/manifest.json (real track metadata, pulled from each patch)..."
python3 - "$STATION" "$STATION_OUT/manifest.json" "${PATCH_PATHS[@]}" << 'PYEOF'
import json, sys

station, out_path, *patch_paths = sys.argv[1:]
tracks = []
for p in patch_paths:
    with open(p) as f:
        data = json.load(f)
    meta = data.get("metadata", {})
    tracks.append({
        "name": meta.get("name", "UNTITLED"),
        "description": meta.get("description", ""),
        "tags": meta.get("tags", []),
    })

with open(out_path, "w") as f:
    json.dump({"slug": station, "trackCount": len(tracks), "tracks": tracks}, f, indent=2)
PYEOF

echo "Done: ${STATION_OUT}/playlist.mp3 (+ manifest.json, ${#WAVS[@]} tracks)"
