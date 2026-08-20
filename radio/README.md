# Murmur Radio (Phase 3 — architecture sketch + proof-of-concept)

A headless MURMUR instance broadcasting ambient pieces via a real, standard
Icecast stream — connectable from "whatever streamer they prefer" (VLC,
mpv, a `<audio>` tag, any Icecast/Shoutcast-compatible client), no custom
protocol.

**Status: v1 proof-of-concept is live.** Real render → crossfade → stream
pipeline, fixed playlist, real audio, real listeners can connect today:

```
http://178.156.212.103:8000/murmur-ambient.mp3
```

This is deliberately the smallest real thing that proves the pipeline
end-to-end — not the full "agentic, showcases community pieces" vision.
See "What's NOT built yet" below for the honest gap list.

## Architecture

```
 build_playlist.sh (offline, run by hand / re-run when the playlist changes)
   ├─ murmur-render × 3   (real DSP, one .murmur preset + .mid each → .wav)
   └─ ffmpeg acrossfade   (chains the 3 WAVs into one continuous playlist.mp3)
                                │
                                ▼
 murmur-radio.service (systemd, always-on, cheap — just loops + remuxes)
   ffmpeg -stream_loop -1 -i playlist.mp3 -c:a copy -f mp3 icecast://...
                                │
                                ▼
 icecast2 (systemd, always-on)
   listens on :8000, real source-password protected, public listener mounts
                                │
                                ▼
 any real Icecast client  ──────┘
```

Two separate systemd services, on purpose: rendering is real, somewhat
expensive DSP work (seconds per patch) that should happen offline / rarely;
looping an already-built MP3 into Icecast is nearly free (no re-encode,
`-c:a copy`) and can run forever.

## Real deployment (Hetzner box, 178.156.212.103)

- `icecast2` — installed via apt, config at `/etc/icecast2/icecast.xml`.
  Real random source/relay/admin passwords generated at setup time (not
  the package's default `hackme` — that was changed before the service was
  ever started, per this project's established secret-handling pattern:
  live only in server-side config, never committed, never echoed in full
  after creation). Listens on `:8000` only — doesn't touch `:80`/`:8080`,
  which nginx already owns for murmur-web/patchforge.
- `murmur-radio.service` — `/etc/systemd/system/murmur-radio.service`,
  `Restart=always`, reads the Icecast source URL (with password) from
  `/etc/default/murmur-radio` (mode 600, not in any repo).
- `/root/murmur-radio/playlist.mp3` — the current built playlist. Rebuild
  with `radio/build_playlist.sh /root/murmur-render-check /root/murmur-radio`
  then `systemctl restart murmur-radio` to pick it up.
- `/root/murmur-render-check` — a real checkout of this repo
  (`cursor/favorites-unison-stack-daw`) with `murmur-render` built
  (`build/render-check/`, `PW8_BUILD_TOOLS=ON`, `PW8_BUILD_TESTS=OFF` —
  no JUCE/GUI dependency, `murmur-render` only links `pw8::core`). Kept
  around specifically so `build_playlist.sh` can be re-run without
  reinstalling the whole toolchain.

## Real de-risking finding from building this (already fixed)

Step 1 of this phase was "verify `murmur-render` actually runs on real
Linux before writing new code" — it didn't. `render()`'s free-function
wrapper stack-allocated a ~19.3MB `Engine` object, instantly overflowing
the default 8MB Linux thread stack (confirmed via `gdb`: SIGSEGV in the
function prologue, before any code ran, on every preset). This was
mis-attributed in earlier project notes to a "macOS sandbox" issue — it
isn't sandbox-related at all, just a universal stack-size bug that never
got exercised on Linux before (the CI fuzz-render job only runs against
`main`, not this work branch). Fixed by heap-allocating via
`std::make_unique<Engine>()` (commit `0adb313`), matching the pattern
`MurmurProcessor.cpp` (the real JUCE plugin) already used for the same
reason. See that commit message for full verification detail.

## What's NOT built yet (honest, per the plan's own scoping)

- **Not agentic.** The playlist is 3 hand-picked presets, fixed at build
  time. No next-track selection logic, no live generation loop. The plan
  explicitly scoped "prove the pipeline first" ahead of any agentic layer.
- **Not showcasing community/curator-approved pieces.** Phase 1 (curator
  mode) and this playlist aren't wired together yet — real work, deferred
  per the plan ("eventually," once there's real curated content worth
  showcasing).
- **No TLS / custom domain.** Stream is plain HTTP on the bare IP. Once
  `murmurmusic.ai` DNS + TLS work happens (separately deferred, per
  explicit user instruction), this could get a real `radio.murmurmusic.ai`
  proxy — not done now, out of scope for the PoC.
- **No loudness normalization across tracks.** The three renders have
  different natural peak/RMS (0.91/0.79/0.65 peak respectively) — audible
  level jumps between tracks are real and unaddressed. A real v2 would
  run a loudness pass (e.g. `ffmpeg loudnorm`) per render before
  crossfading.
- **Single fixed loop, not a rotating catalog.** `playlist.mp3` is one
  static file; a real "radio station" would need a longer/rotating
  source, which this architecture supports (rebuild + restart) but
  doesn't yet automate.
