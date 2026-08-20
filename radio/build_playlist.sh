#!/usr/bin/env bash
# Murmur Radio v1 -- fixed real playlist builder (Phase 3 proof-of-concept).
#
# Renders a small, hand-picked set of real ambient/pad presets through the
# real DSP engine (murmur-render), then crossfades them together into one
# continuous, seamless-loop MP3 with ffmpeg's `acrossfade` filter. The
# result is what murmur-radio.service (see radio/README.md) loops forever
# into Icecast -- this script is the offline "build" step, not something
# that runs inside the always-on service itself (rendering is real DSP
# work; looping an already-built file is cheap).
#
# Requires murmur-render already built (PW8_BUILD_TOOLS=ON) and ffmpeg on
# PATH. Real IMPORTANT gotcha this script's callers must know: murmur-render
# resolves every patch's relative paths (wavetableId, etc.) against its own
# working directory, so this script always shells out from REPO_ROOT --
# same reason mcp_server/render.py does the same thing.
#
# Usage: radio/build_playlist.sh <repo_root> <output_dir>
#   e.g. radio/build_playlist.sh /root/murmur-render-check /root/murmur-radio

set -euo pipefail

REPO_ROOT="${1:?Usage: build_playlist.sh <repo_root> <output_dir>}"
OUT_DIR="${2:?Usage: build_playlist.sh <repo_root> <output_dir>}"
RENDERER="${REPO_ROOT}/build/render-check/tools/murmur-render"

if [[ ! -x "$RENDERER" ]]; then
  echo "murmur-render not found/executable at $RENDERER -- build it first:" >&2
  echo "  cmake -S . -B build/render-check -G Ninja -DCMAKE_BUILD_TYPE=Release -DPW8_BUILD_TESTS=OFF -DPW8_BUILD_TOOLS=ON" >&2
  echo "  cmake --build build/render-check -j2 --target murmur-render" >&2
  exit 1
fi

# Real v1 fixed playlist -- three real factory/showcase ambient patches,
# picked by ear from content/presets for genuinely different character
# (bright pad / warm pad / dark drift) rather than three similar-sounding
# pieces. "Agentic" next-track selection (per the plan) is explicitly out
# of scope for this first pass -- prove render -> crossfade -> stream works
# end-to-end before layering selection logic on top.
declare -a PATCHES=(
  "content/presets/aurora-pad.murmur:content/test_midi/pad-chords.mid:01-aurora-pad"
  "content/presets/halcyon-pad.murmur:content/test_midi/tide_pad.mid:02-halcyon-pad"
  "content/presets/factory/Ambient/04-fathomless-drift.murmur:content/test_midi/bloom_pad.mid:03-fathomless-drift"
)

mkdir -p "${OUT_DIR}/renders"
cd "$REPO_ROOT"

WAVS=()
for entry in "${PATCHES[@]}"; do
  IFS=':' read -r patch midi label <<< "$entry"
  out="${OUT_DIR}/renders/${label}.wav"
  echo "Rendering ${label} (${patch})..."
  "$RENDERER" --patch "$patch" --midi "$midi" --output "$out" --duration 45 --release-tail 3
  WAVS+=("$out")
done

# 4-second triangular crossfades, chained pairwise (ffmpeg's acrossfade only
# takes two inputs at a time) -- real overlap, not a hard cut, matching
# "crossfade between different arrangements" from the original ask.
echo "Crossfading ${#WAVS[@]} renders into ${OUT_DIR}/playlist.mp3..."
ffmpeg -y \
  -i "${WAVS[0]}" -i "${WAVS[1]}" -i "${WAVS[2]}" \
  -filter_complex "[0][1]acrossfade=d=4:c1=tri:c2=tri[a01];[a01][2]acrossfade=d=4:c1=tri:c2=tri[out]" \
  -map "[out]" -ar 44100 -c:a libmp3lame -b:a 192k "${OUT_DIR}/playlist.mp3"

echo "Done: ${OUT_DIR}/playlist.mp3"
