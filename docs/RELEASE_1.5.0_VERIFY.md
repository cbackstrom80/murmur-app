# MURMUR 1.5.0 — Release verification checklist

Run before sending the DMG to Ben.

## Build gate

```bash
scripts/release_gate.sh 1.5.0
```

Expected: `PASS — Cadillac release ready for Ben`

Artifacts in `dist/`:

- [ ] `MURMUR-1.5.0-macOS-arm64-full.pkg`
- [ ] `MURMUR-1.5.0-macOS-arm64-full.dmg`
- [ ] `SHA256SUMS.txt`
- [ ] `version.json`

## Automated checks (in release_gate)

- [ ] `validate_content_refs.py` — all wavetable refs resolve
- [ ] Factory presets ≥ 1000 (Interstellar ≥ 100, Hoover ≥ 28)
- [ ] `auval -v aumu Murm Murr` — PASS
- [ ] Standalone bundle ID ≠ AU (`com.patchwork.murmur.standalone` vs `com.patchwork.murmur`)

## Fresh-install test

1. [ ] Remove old install: `rm -rf ~/Library/Audio/Plug-Ins/Components/MURMUR.component ~/Applications/MURMUR.app`
2. [ ] Run `Install MURMUR.pkg` from DMG — no admin password
3. [ ] **AU exists:** `ls ~/Library/Audio/Plug-Ins/Components/MURMUR.component`
4. [ ] AU version **1.5.0**
5. [ ] Factory presets ≥ 1129

## Logic Pro soak

1. [ ] Quit Logic, reopen, Reset & Rescan
2. [ ] MURMUR loads — no crash
3. [ ] **⌘B** — browser opens on **ALL PRESETS**
4. [ ] Category **INTERSTELLAR** → CATHEDRAL NEBULA loads
5. [ ] **DESIGN → FX** — card browser + chain view render
6. [ ] Open an FX detail — hero viz + knob strip visible

## Sign-off

| | Name | Date |
|---|------|------|
| Build | | |
| Logic test | | |
| Ben ship | | |
