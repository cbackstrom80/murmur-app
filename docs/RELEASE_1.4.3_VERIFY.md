# MURMUR 1.4.3 — Release verification checklist

Run before sending the DMG to Ben.

## Build gate

```bash
scripts/release_gate.sh 1.4.3
```

Expected: `PASS — Cadillac release ready for Ben`

Artifacts in `dist/`:

- [ ] `MURMUR-1.4.3-macOS-arm64-full.pkg`
- [ ] `MURMUR-1.4.3-macOS-arm64-full.dmg`
- [ ] `SHA256SUMS.txt`
- [ ] `version.json`

## Automated checks (in release_gate)

- [ ] `validate_content_refs.py` — all wavetable refs resolve
- [ ] Factory presets ≥ 1000 (Interstellar ≥ 100)
- [ ] `auval -v aumu Murm Murr` — PASS
- [ ] pluginval strictness 5 — PASS (if cask installed)

## Fresh-install test (clean Mac or test user)

1. [ ] Open DMG — readme files present (`1 — READ ME FIRST.txt`, `INSTALL.txt`, `WHATS NEW.txt`)
2. [ ] Run `Install MURMUR.pkg` — completes without admin password
3. [ ] Confirm AU version: `plutil -p ~/Library/Audio/Plug-Ins/Components/MURMUR.component/Contents/Info.plist | grep Version` → **1.4.3**
4. [ ] Factory presets: `find ~/Library/Application\ Support/MURMUR/Presets/factory -name '*.pw8' | wc -l` → **≥ 1000**

## Logic Pro soak

1. [ ] Quit Logic completely, reopen
2. [ ] Plug-in Manager → Reset & Rescan Selection
3. [ ] Load MURMUR on Software Instrument track — **no crash**
4. [ ] Version shows **1.4.3** in plug-in
5. [ ] PLAY mode: scope + master deck side-by-side, 6 macros
6. [ ] **BROWSE** / ⌘B opens Preset Explorer
7. [ ] Load factory preset — sound + wavetables resolve
8. [ ] Prev/next preset chevrons work
9. [ ] Save/recall project with MURMUR state

## Optional

- [ ] Standalone `MURMUR.app` launches from DMG
- [ ] Kawai MP11SE CC map per `Docs/KAWAI_MP11SE.md`

## Sign-off

| | Name | Date |
|---|------|------|
| Build | | |
| Logic test | | |
| Ben ship | | |
