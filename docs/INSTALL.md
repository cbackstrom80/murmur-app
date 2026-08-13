# Installing MURMUR on macOS (Ben MVP)

**MURMUR 1.0.6.1** — install like any Mac app: download the `.pkg`, double-click, open Logic. No Xcode or terminal required.

Built for **Apple Silicon (M1/M2/M3/M4)** + **Logic Pro** + **Kawai MP11SE**.

---

## Quick install

1. **Download** from [GitHub Releases](https://github.com/cbackstrom80/patchwork-eight/releases):
   - `MURMUR-1.0.6.1-macOS-arm64.pkg`

2. **Open the `.pkg`** and follow the installer.
   - Installs to **your home folder** — **no admin password**
   - Includes Audio Unit, 900 factory presets, wavetables, and Logic/MP11SE docs

3. **Quit Logic completely**, then reopen.

4. **Rescan plug-ins:** Logic → **Settings → Plug-in Manager → Reset & Rescan Selection**

5. **Add MURMUR:** new Software Instrument track → **AU Instruments → Murmur → MURMUR**

6. **Load a preset:** **BROWSE** or **LOAD...** in the plug-in PLAY screen.

Full Ben MVP guide: [`BEN_MVP.md`](BEN_MVP.md)

---

## Logic Pro setup

| Topic | Doc |
|-------|-----|
| **MURMUR product docs** | [`product/README.md`](product/README.md) |
| MP11SE knob + pedal mapping | [`KAWAI_MP11SE.md`](KAWAI_MP11SE.md) |
| Smart Controls 8-knob template | [`LOGIC_SMART_CONTROLS.md`](LOGIC_SMART_CONTROLS.md) |
| All MIDI CC / mod wheel / expression | [`MIDI_CONTROLLERS.md`](MIDI_CONTROLLERS.md) |

Installed copies live at:

```
~/Library/Application Support/MURMUR/Docs/
~/Library/Application Support/MURMUR/Docs/product/
```

---

## What gets installed

| Item | Path |
|------|------|
| Audio Unit | `~/Library/Audio/Plug-Ins/Components/MURMUR.component` |
| Factory presets (900) | `~/Library/Application Support/MURMUR/Presets/factory/` |
| Showcase presets | `~/Library/Application Support/MURMUR/Presets/showcase/` |
| Wavetables | `~/Library/Application Support/MURMUR/Wavetables/` |
| Docs (Logic, MP11SE, MIDI) | `~/Library/Application Support/MURMUR/Docs/` |
| Product documentation | `~/Library/Application Support/MURMUR/Docs/product/` |

The default Ben release is **Audio Unit only** (ideal for Logic). Maintainers can build VST3 + Standalone with `scripts/build_release_pkg.sh --full`.

---

## Gatekeeper

Release builds are **ad-hoc signed** until Apple Developer ID is configured.

If macOS blocks the installer:

1. **Right-click** the `.pkg` → **Open** → confirm **Open**
2. After install, if the AU fails: **System Settings → Privacy & Security** → allow if prompted
3. Remove quarantine if needed: `xattr -dr com.apple.quarantine ~/Downloads/MURMUR-*.pkg`

---

## Updating

1. Download the latest `.pkg` from [Releases](https://github.com/cbackstrom80/patchwork-eight/releases)
2. Run it (overwrites previous install)
3. Restart Logic

---

## Building the installer (maintainers)

```bash
scripts/build_release_pkg.sh 1.0.0          # Ben pkg (AU + content, user Library)
scripts/build_release_pkg.sh --dmg 1.0.0    # + DMG wrapper
scripts/build_release_pkg.sh --full --target system 1.0.0   # VST3 + Standalone, all users
```

Output: `dist/MURMUR-<version>-macOS-arm64.pkg` and `dist/version.json`

CI release: push tag `v1.0.0` → `.github/workflows/release.yml`

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Logic doesn’t see MURMUR | Quit Logic fully, reinstall pkg, Reset & Rescan in Plug-in Manager |
| Presets empty in BROWSE | Confirm `~/Library/Application Support/MURMUR/Presets/factory/` has `.pw8` files |
| Pads stack on legato chords | Reinstall — Ben MVP includes pad polyphony + legato fix |
| MP11SE knobs dead | Program MIDI OUT zone CCs per `Docs/KAWAI_MP11SE.md` |
| “Cannot verify developer” | Right-click → Open on the `.pkg` |
